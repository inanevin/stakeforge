/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "editor_world_controller.hpp"
#include "editor_app.hpp"
#include "editor_surface_controller.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_manager_util.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_writer.hpp"
#include "editor_command_system.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include "world/editor_world.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
	void editor_world_controller_t::init()
	{
		s_instance = this;

		_worlds.reserve(8);
		_render_worlds.reserve(8);

		_asset_deletion_listener = editor_asset_manager_t::get().add_asset_deletion_listener(on_asset_deletion, this);
		_command_listener		 = editor_command_system_t::get().add_listener(on_command_system_event, this);

		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		_play_main_world_dirty = false;
	}

	void editor_world_controller_t::uninit()
	{
		if (!_asset_deletion_listener.is_null())
		{
			editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);
			_asset_deletion_listener = {};
		}

		if (!_command_listener.is_null())
		{
			editor_command_system_t::get().remove_listener(_command_listener);
			_command_listener = {};
		}

		destroy_worlds_internal(false);
		_render_worlds.resize(0);

		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		s_instance = nullptr;
	}

	editor_world_handle_t editor_world_controller_t::create_world(const world_init_config_t& init_config, editor_world_edit_type_e edit_type, editor_world_tick_callback_t tick_callback, void* tick_callback_user_data)
	{
		editor_app_t::get().stop_render();

		world_init_config_t		  world_config	   = init_config;
		const project_settings_t& project_settings = engine_runtime_t::get().get_project_settings();
		world_config.physics					   = project_settings.physics.make_runtime_config(project_settings.world_physics_rate, project_settings.max_sim_steps);

		const editor_world_handle_t handle = _worlds.add();
		_worlds.get(handle)				   = new editor_world_t();
		editor_world_t* const editor_world = _worlds.get(handle);

		editor_world->init(world_config, handle, edit_type, tick_callback, tick_callback_user_data);

		return handle;
	}

	void editor_world_controller_t::destroy_world(editor_world_handle_t handle)
	{
		editor_app_t::get().stop_render();

		const bool was_main_world = _main_world == handle;

		if (was_main_world)
		{
			stop_main_world_play_mode();
			set_main_world({}, NULL_SID, "");
		}

		destroy_world_internal(handle);
	}

	void editor_world_controller_t::destroy_world_internal(editor_world_handle_t handle)
	{
		editor_world_t* const world = _worlds.get(handle);

		world->uninit();
		delete world;

		_worlds.remove(handle);
	}

	void editor_world_controller_t::destroy_main_world_internal()
	{
		if (_main_world.is_null())
			return;

		stop_main_world_play_mode();

		const editor_world_handle_t handle = _main_world;
		set_main_world({}, NULL_SID, "");
		destroy_world_internal(handle);
	}

	void editor_world_controller_t::destroy_worlds()
	{
		destroy_worlds_internal(true);
	}

	void editor_world_controller_t::destroy_worlds_internal(bool notify_panels)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		stop_main_world_play_mode();

		if (notify_panels && !_main_world.is_null())
			set_main_world({}, NULL_SID, "");

		for (editor_world_t* world : _worlds)
		{
			world->uninit();
			delete world;
		}

		_worlds.resize_zero();
		_main_world					   = {};
		_main_world_asset_guid		   = NULL_SID;
		_pending_main_world_asset_guid = NULL_SID;
		_main_world_name.resize(0);
		_main_world_dirty	   = false;
		_play_main_world_dirty = false;
	}

	void editor_world_controller_t::stop_main_world_play_mode()
	{
		if (_main_world.is_null())
			return;

		editor_world_t* const		 editor_world = _worlds.get(_main_world);
		editor_world_edit_context_t& context	  = editor_world->get_edit_context();
		context.set_play_mode(editor_play_mode_e::none);
		context.set_do_step(false);

		if (editor_world->get_play_mode() == editor_play_mode_e::none)
			return;

		update_main_world_play_mode(editor_play_mode_e::none);
	}

	void editor_world_controller_t::resize_world(editor_world_handle_t handle, vec2u16_t render_resolution)
	{
		editor_world_t* const world = _worlds.get(handle);

		if (world->get_render_resolution() == render_resolution)
			return;

		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		world->resize(render_resolution);
	}

	bool editor_world_controller_t::acquire_render_worlds()
	{
		ZoneScoped;

		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());
		_render_worlds.resize(0);

		if (_worlds.empty())
			return false;

		_render_alpha = calculate_render_alpha();

		for (editor_world_t* world : _worlds)
		{
			_render_worlds.push_back({
				.world	  = world,
				.snapshot = &world->acquire_render_snapshot(),
			});
		}

		return true;
	}

	bool editor_world_controller_t::render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		if (_render_worlds.empty())
			return false;

		for (const acquired_render_world_t& acquired : _render_worlds)
			acquired.world->render(*acquired.snapshot, _render_alpha, frame_index, global_cbv_index, global_layout);

		_render_worlds.resize(0);

		gfx_backend& backend = gfx_backend::get();
		backend.queue_signal(queue, &signal, &signal_value, 1);
		return true;
	}

	void editor_world_controller_t::tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps)
	{
		ZoneScoped;

		editor_play_mode_e play_mode = editor_play_mode_e::none;
		bool			   do_step	 = false;

		if (!_main_world.is_null())
		{
			editor_world_edit_context_t& context = _worlds.get(_main_world)->get_edit_context();
			play_mode							 = context.get_play_mode();
			do_step								 = context.is_do_step();
			context.set_do_step(false);
		}

		update_main_world_play_mode(play_mode);

		for (editor_world_t* editor_world : _worlds)
			editor_world->begin_frame();

		const i64 now				  = time_t::get_cpu_microseconds();
		const i64 delta_us			  = now - _previous_time_us;
		const f32 frame_delta_seconds = static_cast<f32>(delta_us) / 1000000.0f;
		_previous_time_us			  = now;

		u32		  steps	   = 0;
		const i64 fixed_us = world_tick_rate == 0 ? 0 : 1000000 / static_cast<i64>(world_tick_rate);

		if (fixed_us > 0)
		{
			_accumulator_us += delta_us;

			const f32 dt_seconds = 1.0f / static_cast<f32>(world_tick_rate);

			while (_accumulator_us >= fixed_us && steps < max_sim_steps)
			{
				_accumulator_us -= fixed_us;

				for (auto it = _worlds.begin_handle(); it != _worlds.end_handle(); ++it)
				{
					const editor_world_handle_t handle		 = *it;
					editor_world_t*				editor_world = _worlds.get(handle);
					tick_editor_world(*editor_world, handle == _main_world, play_mode, dt_seconds, false);
				}

				++steps;
			}

			if (do_step && !_main_world.is_null() && (play_mode == editor_play_mode_e::play_paused || play_mode == editor_play_mode_e::play_physics_paused))
				tick_editor_world(*_worlds.get(_main_world), true, play_mode, dt_seconds, true);

			_fixed_step_us.store(fixed_us, std::memory_order_relaxed);
			_last_fixed_step_us.store(now - _accumulator_us, std::memory_order_release);
		}
		else
		{
			_accumulator_us = 0;
			_fixed_step_us.store(0, std::memory_order_relaxed);
			_last_fixed_step_us.store(now, std::memory_order_release);
		}

		for (auto it = _worlds.begin_handle(); it != _worlds.end_handle(); ++it)
		{
			const editor_world_handle_t handle			  = *it;
			editor_world_t* const		editor_world	  = _worlds.get(handle);
			const bool					main_world_paused = handle == _main_world && (play_mode == editor_play_mode_e::play_paused || play_mode == editor_play_mode_e::play_physics_paused);

			if (steps == 0 && !main_world_paused)
				editor_world->update_world_transforms(false);

			editor_world->invoke_tick_callback(frame_delta_seconds);
			editor_world->draw_debug();

#ifdef SFG_DEBUG
			world_t& world = editor_world->get_world();

			world.get_debug_draw().debug_draw_missing_resources(world);
#endif

			editor_world->produce_snapshot();
		}
	}

	void editor_world_controller_t::update_main_world_play_mode(editor_play_mode_e mode)
	{
		if (_main_world.is_null())
			return;

		editor_world_t&			 editor_world  = *_worlds.get(_main_world);
		const editor_play_mode_e previous_mode = editor_world.get_play_mode();

		if (previous_mode == mode)
			return;

		const bool entering_play = previous_mode == editor_play_mode_e::none;
		const bool exiting_play	 = mode == editor_play_mode_e::none;
		const bool entering_full = entering_play && mode == editor_play_mode_e::play;
		const bool exiting_full	 = exiting_play && (previous_mode == editor_play_mode_e::play || previous_mode == editor_play_mode_e::play_paused);

		if (entering_play)
			_play_main_world_dirty = _main_world_dirty;

		editor_world.update_play_mode(mode);

		if (entering_full)
			editor_surface_controller_t::get().set_play_cursor_locked(true);
		else if (exiting_full)
			editor_surface_controller_t::get().set_play_cursor_locked(false);

		if (!exiting_play)
			return;

		_main_world_dirty	   = _play_main_world_dirty;
		_play_main_world_dirty = false;
		notify_main_world_dirty_changed();

		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::entities))
			static_cast<editor_panel_entities_t*>(panel)->refresh_entities();

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::inspector))
			static_cast<editor_panel_inspector_t*>(panel)->refresh_from_selection();
	}

	void editor_world_controller_t::tick_editor_world(editor_world_t& editor_world, bool is_main_world, editor_play_mode_e mode, f32 dt_seconds, bool force_simulation)
	{
		world_t& world = editor_world.get_world();

		if (!is_main_world)
		{
			editor_world.tick_camera(dt_seconds);
			world.update_world_transforms();
			world.tick_physics(dt_seconds);

			world.tick_animation_prep(dt_seconds);
			world.tick_post();
			return;
		}

		switch (mode)
		{
		case editor_play_mode_e::none:
			editor_world.tick_camera(dt_seconds);
			world.update_world_transforms();

			world.tick_animation_prep(dt_seconds);
			world.tick_animation_logic(dt_seconds);

			break;
		case editor_play_mode_e::play:
			world.update_world_transforms();
			world.tick_physics(dt_seconds);

			world.tick_animation_prep(dt_seconds);
			world.tick_animation_logic(dt_seconds);

			break;
		case editor_play_mode_e::play_physics:
			editor_world.tick_camera(dt_seconds);
			world.update_world_transforms();
			world.tick_physics(dt_seconds);
			break;
		case editor_play_mode_e::play_paused:
			if (force_simulation)
			{
				world.update_world_transforms();
				world.tick_physics(dt_seconds);

				world.tick_animation_prep(dt_seconds);
				world.tick_animation_logic(dt_seconds);
			}

			break;
		case editor_play_mode_e::play_physics_paused:
			if (force_simulation)
			{
				editor_world.tick_camera(dt_seconds);
				world.update_world_transforms();
				world.tick_physics(dt_seconds);
			}

			break;
		}

		world.tick_post();
	}

	void editor_world_controller_t::update_physics_settings(const u64* collision_masks, u64 active_layers, u32 physics_rate, u32 max_sub_steps)
	{
		for (editor_world_t* editor_world : _worlds)
		{
			world_t& world = editor_world->get_world();

			if (!world.get_physics().is_init())
				continue;

			physics_world_t& physics = world.get_physics();
			physics.update_collision_masks(collision_masks, active_layers);
			physics.update_step_settings(physics_rate, max_sub_steps);
		}
	}

	void editor_world_controller_t::install_default_world(editor_world_handle_t handle)
	{
		editor_world_t* const editor_world = _worlds.get(handle);

		editor_world->install_camera(editor_world_camera_type_e::fly);
		world_t& world = editor_world->get_world();

		const entity_id_t	environment = world.create_entity("environment");
		component_skybox_t& skybox		= ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(type_id_t<component_skybox_t>::value), environment);
		skybox.skybox_asset				= DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		skybox.exposure					= 0.25f;

		ecs_component_table_t& debug_widgets_table	= world.get_component_table(type_id_t<component_debug_widgets_t>::value);
		const bool			   debug_widgets_exists = ecs_t::table_has(debug_widgets_table, environment);
		void*				   debug_widgets		= ecs_t::table_add(debug_widgets_table, environment);

		if (!debug_widgets_exists)
		{
			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(debug_widgets_table.type_desc.type_id);
			SFG_ASSERT(reflected_type != nullptr && reflected_type->default_init_fn != nullptr);
			reflected_type->default_init_fn(debug_widgets);
		}

		world.add_resource(resource_type_e::hdr_skybox, DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID);
		world.load_all_used_resources();
	}

	bool editor_world_controller_t::load_main_world(sid_t asset_guid)
	{
		if (!_main_world.is_null() && _worlds.get(_main_world)->get_edit_context().get_play_mode() != editor_play_mode_e::none)
			return false;

		editor_app_t::get().stop_render();

		if (!_main_world.is_null() && _main_world_dirty)
		{
			_pending_main_world_asset_guid		 = asset_guid;
			editor_modal_button_desc_t buttons[] = {
				{.text = "Save", .callback = on_save_dirty_world_modal, .user_data = this},
				{.text = "Don't Save", .callback = on_dont_save_dirty_world_modal, .user_data = this},
				{.text = "Cancel", .callback = on_cancel_dirty_world_modal, .user_data = this},
			};
			editor_surface_controller_t::get().get_main_surface().modal_controller->request_modal("Save World", "Would you like to save the current world.", buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), editor_modal_severity_e::warning);
			return true;
		}

		return load_main_world_now(asset_guid);
	}

	void editor_world_controller_t::load_dummy_world()
	{
		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_main_world_internal();

		const world_init_config_t init_config{
			.render_resolution				= editor_surface_controller_t::get().get_main_surface().swapchain_size,
			.render_entity_max				= 1024 * 10,
			.render_bone_max				= 4096,
			.render_bone_reserve			= 1024,
			.animation_graph_memory_reserve = 1 * 1024 * 1024,
			.component_table_reserve		= 64,
			.free_list_reserve				= 1024,
			.used_resource_reserve			= 512,
			.text_allocation_reserve		= 1024,
			.physics_enabled				= true,
		};

		const editor_world_handle_t handle = create_world(init_config);
		install_default_world(handle);
		set_main_world(handle, NULL_SID, "unnamed");
	}

	bool editor_world_controller_t::load_main_world_now(sid_t asset_guid)
	{
		editor_app_t::get().stop_render();

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(asset_guid);

		if (asset == nullptr || asset->asset_type != editor_asset_type_e::world)
		{
			SFG_ERR("failed to find world asset {0}", asset_guid);
			return false;
		}

		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_main_world_internal();

		const world_init_config_t init_config{
			.render_resolution				= editor_surface_controller_t::get().get_main_surface().swapchain_size,
			.render_entity_max				= 1024 * 10,
			.render_bone_max				= 4096,
			.render_bone_reserve			= 1024,
			.animation_graph_memory_reserve = 1 * 1024 * 1024,
			.component_table_reserve		= 64,
			.free_list_reserve				= 1024,
			.used_resource_reserve			= 512,
			.text_allocation_reserve		= 1024,
			.physics_enabled				= true,
		};

		const editor_world_handle_t handle = create_world(init_config);

		if (asset->embedded_source.empty())
			install_default_world(handle);
		else
		{
			editor_world_t* const editor_world	  = _worlds.get(handle);
			world_t&			  world			  = editor_world->get_world();
			const nlohmann::json  embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			world_cooker_t::world_from_json(world, embedded_source);
			// world.load_all_used_resources();
			editor_world->install_camera(editor_world_camera_type_e::fly);
			editor_world->deserialize_camera(embedded_source.value<nlohmann::json>("editor_camera", nlohmann::json::object()));
		}

		const char* asset_name = editor_asset_util_t::find_asset_display_name(asset_guid);
		set_main_world(handle, asset_guid, asset_name != nullptr ? asset_name : "unnamed");

		if (!asset->embedded_source.empty())
		{
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			_worlds.get(_main_world)->get_edit_context().read_folders_from_json(embedded_source.value<nlohmann::json>("folders", nlohmann::json::array()));

			if (editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::entities))
				static_cast<editor_panel_entities_t*>(panel)->refresh_entities();
		}

		editor_project_t& project		 = editor_project_t::get();
		project.settings.last_world_guid = asset_guid;
		project.save(project._runtime.path.c_str());
		return true;
	}

	bool editor_world_controller_t::save_main_world()
	{
		if (_main_world.is_null())
			return false;

		nlohmann::json		  world_json   = nlohmann::json::object();
		editor_world_t* const editor_world = _worlds.get(_main_world);

		if (editor_world->get_edit_context().get_play_mode() != editor_play_mode_e::none)
			return false;

		world_cooker_t::world_to_json(editor_world->get_world(), world_json);

		editor_world->get_edit_context().write_folders_to_json(world_json["folders"]);
		editor_world->serialize_camera(world_json["editor_camera"]);

		editor_asset_t		  asset			 = {};
		string_t			  asset_path	 = {};
		const editor_asset_t* existing_asset = _main_world_asset_guid != NULL_SID ? editor_asset_manager_t::get().find_asset(_main_world_asset_guid) : nullptr;

		if (existing_asset != nullptr && existing_asset->asset_type == editor_asset_type_e::world)
		{
			asset_path = editor_asset_util_t::find_asset_path(_main_world_asset_guid);

			if (!asset_path.empty() && file_system_t::exists(asset_path.c_str()))
				asset = *existing_asset;
		}

		if (asset_path.empty() || !file_system_t::exists(asset_path.c_str()))
		{
			asset_path = process::save_file("Save World", "sfg_asset");

			if (asset_path.empty())
			{
				SFG_ERR("world save cancelled");
				return false;
			}

			file_system_t::fix_path(asset_path);

			if (file_system_t::get_file_extension(asset_path) != "sfg_asset")
				asset_path += ".sfg_asset";

			if (!editor_asset_path_t::is_source_inside_assets(editor_project_t::get()._runtime.assets_path.c_str(), asset_path.c_str()))
			{
				SFG_ERR("world save path is outside project assets directory {0}", asset_path.c_str());
				return false;
			}
		}

		const string_t							 parent_path = file_system_t::get_directory_of_file(asset_path.c_str());
		const string_t							 asset_name	 = file_system_t::remove_extensions_from_path(file_system_t::get_filename_and_extension_from_path(asset_path));
		const editor_asset_write_embedded_desc_t write_desc{
			.embedded_source = &world_json,
			.parent_path	 = parent_path.c_str(),
			.name			 = asset_name.c_str(),
			.guid			 = asset.guid != NULL_SID ? asset.guid : _main_world_asset_guid,
			.asset_type		 = editor_asset_type_e::world,
			.allow_overwrite = true,
		};

		if (!editor_asset_writer_t::write_embedded_asset(write_desc, &asset, &asset_path))
		{
			SFG_ERR("failed to save world asset {0}", asset_path.c_str());
			return false;
		}

		_main_world_asset_guid = asset.guid;
		set_main_world_dirty(false);

		editor_project_t& project		 = editor_project_t::get();
		project.settings.last_world_guid = asset.guid;
		project.save(project._runtime.path.c_str());

		editor_asset_manager_t&			 asset_manager = editor_asset_manager_t::get();
		const editor_asset_node_handle_t existing_node = asset_manager.find_asset_node_handle(asset.guid);

		if (!existing_node.is_null())
			asset_manager.reload_asset_node(existing_node);
		else
		{
			const editor_asset_node_handle_t parent_node = asset_manager.find_node_by_path(parent_path.c_str());

			if (!parent_node.is_null())
				asset_manager.add_path_node(parent_node, asset_path.c_str());
		}

		return true;
	}

	void editor_world_controller_t::set_main_world(editor_world_handle_t handle, sid_t asset_guid, const char* name)
	{
		if (_main_world == handle && _main_world_asset_guid == asset_guid && _main_world_name == name)
			return;

		const editor_world_handle_t previous_main_world = _main_world;

		if (!previous_main_world.is_null())
			editor_command_system_t::get().clear_world(previous_main_world);

		_main_world					   = handle;
		_main_world_asset_guid		   = asset_guid;
		_pending_main_world_asset_guid = NULL_SID;
		_main_world_name			   = name;
		_main_world_dirty			   = false;
		notify_main_world_changed();
		notify_main_world_dirty_changed();
	}

	void editor_world_controller_t::set_main_world_dirty(bool dirty)
	{
		if (_main_world.is_null() || _main_world_dirty == dirty)
			return;

		_main_world_dirty = dirty;
		notify_main_world_dirty_changed();
	}

	void editor_world_controller_t::mark_world_dirty(editor_world_handle_t handle)
	{
		if (handle == _main_world)
			set_main_world_dirty(true);
	}

	void editor_world_controller_t::on_save_dirty_world_modal(void* user_data)
	{
		editor_world_controller_t& controller	  = *static_cast<editor_world_controller_t*>(user_data);
		const sid_t				   asset_guid	  = controller._pending_main_world_asset_guid;
		controller._pending_main_world_asset_guid = NULL_SID;

		if (asset_guid != NULL_SID && controller.save_main_world())
			controller.load_main_world_now(asset_guid);
	}

	void editor_world_controller_t::on_dont_save_dirty_world_modal(void* user_data)
	{
		editor_world_controller_t& controller	  = *static_cast<editor_world_controller_t*>(user_data);
		const sid_t				   asset_guid	  = controller._pending_main_world_asset_guid;
		controller._pending_main_world_asset_guid = NULL_SID;

		if (asset_guid != NULL_SID)
			controller.load_main_world_now(asset_guid);
	}

	void editor_world_controller_t::on_cancel_dirty_world_modal(void* user_data)
	{
		static_cast<editor_world_controller_t*>(user_data)->_pending_main_world_asset_guid = NULL_SID;
	}

	void editor_world_controller_t::on_asset_deletion(editor_asset_manager_t&, span_t<const sid_t> asset_ids, void* user_data)
	{
		editor_world_controller_t& controller		  = *static_cast<editor_world_controller_t*>(user_data);
		bool					   main_world_deleted = false;

		for (size_t i = 0; i < asset_ids.size; ++i)
		{
			if (asset_ids.data[i] == controller._main_world_asset_guid)
			{
				main_world_deleted = true;
				break;
			}
		}

		if (!main_world_deleted)
			return;

		controller.destroy_world(controller._main_world);

		editor_project_t& project		 = editor_project_t::get();
		project.settings.last_world_guid = NULL_SID;
		project.save(project._runtime.path.c_str());
	}

	void editor_world_controller_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_world_controller_t& controller = *static_cast<editor_world_controller_t*>(user_data);

		switch (command.type)
		{
		case editor_command_type_e::entity_create:
		case editor_command_type_e::entity_duplicate:
		case editor_command_type_e::entity_destroy:
		case editor_command_type_e::entity_reparent:
		case editor_command_type_e::prefab_spawn:
		case editor_command_type_e::primitive_spawn:
		case editor_command_type_e::entity_info_paste:
		case editor_command_type_e::entity_info_edit:
		case editor_command_type_e::component_add:
		case editor_command_type_e::component_remove:
		case editor_command_type_e::component_reset:
		case editor_command_type_e::component_paste:
		case editor_command_type_e::component_edit:
		case editor_command_type_e::world_edit_context_create_folder:
		case editor_command_type_e::world_edit_context_rename_folder:
		case editor_command_type_e::world_edit_context_color_folder:
		case editor_command_type_e::world_edit_context_assign_folder:
		case editor_command_type_e::world_edit_context_assign_folder_parent:
			controller.set_main_world_dirty(true);
			break;
		default:
			break;
		}
	}

	void editor_world_controller_t::notify_main_world_changed()
	{
		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::world))
		{
			editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
			world_panel->set_edit_world(_main_world);
			world_panel->set_panel_name(_main_world.is_null() ? "" : _main_world_name.c_str());
		}

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::entities))
		{
			editor_panel_entities_t* entities_panel = static_cast<editor_panel_entities_t*>(panel);
			entities_panel->set_edit_world(_main_world);
			entities_panel->refresh_entities();
		}

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::inspector))
		{
			editor_panel_inspector_t* inspector_panel = static_cast<editor_panel_inspector_t*>(panel);
			inspector_panel->set_edit_world(_main_world);
			inspector_panel->refresh_from_selection();
		}
	}

	void editor_world_controller_t::notify_main_world_dirty_changed()
	{
		if (editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::world))
			static_cast<editor_panel_world_t*>(panel)->set_world_dirty(_main_world_dirty);
	}

	f32 editor_world_controller_t::calculate_render_alpha() const
	{
		const i64 fixed_us = _fixed_step_us.load(std::memory_order_relaxed);

		if (fixed_us <= 0)
			return 0.0f;

		const i64 last_fixed_step_us = _last_fixed_step_us.load(std::memory_order_acquire);
		const i64 now				 = time_t::get_cpu_microseconds();
		const f32 alpha				 = static_cast<f32>(static_cast<double>(now - last_fixed_step_us) / static_cast<double>(fixed_us));

		if (alpha < 0.0f)
			return 0.0f;

		if (alpha > 1.0f)
			return 1.0f;

		return alpha;
	}
}
