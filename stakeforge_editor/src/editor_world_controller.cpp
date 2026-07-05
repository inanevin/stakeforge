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
#include "assets/editor_asset_util.hpp"
#include "editor_app.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define WORLD_SNAPSHOT_SLOT_COUNT		3
#define WORLD_SNAPSHOT_SLOT_MASK		0x3
#define WORLD_SNAPSHOT_FRESH_FLAG		0x80
#define EDITOR_CAMERA_BASE_MOVE_SPEED	12.0f
#define EDITOR_CAMERA_BOOST_MULTIPLIER	8.0f
#define EDITOR_CAMERA_MOUSE_SENSITIVITY 0.2f

	editor_world_controller_t::world_container_t& editor_world_controller_t::world_container_t::operator=(world_container_t&& other) noexcept
	{
		for (u32 i = 0; i < WORLD_SNAPSHOT_SLOT_COUNT; ++i)
			snapshot_slots[i] = static_cast<world_render_snapshot_t&&>(other.snapshot_slots[i]);

		render_context	= static_cast<world_render_context_t&&>(other.render_context);
		world_resources = static_cast<vector_t<u64>&&>(other.world_resources);
		snapshot_mailbox.store(other.snapshot_mailbox.load(std::memory_order_relaxed), std::memory_order_relaxed);
		edit_context  = other.edit_context;
		handle		  = other.handle;
		producer_slot = other.producer_slot;
		consumer_slot = other.consumer_slot;

		other.snapshot_mailbox.store(0, std::memory_order_relaxed);
		other.world_resources.resize(0);
		other.edit_context	= {};
		other.handle		= {};
		other.producer_slot = 0;
		other.consumer_slot = 0;
		return *this;
	}

	void editor_world_controller_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);
		s_instance = this;
		_edit_contexts.reserve(8);
		_command_listener = editor_command_system_t::get().add_listener(on_command_system_event, this);
		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		reset_camera_input();
		_main_camera_entity = NULL_ENTITY_ID;
	}

	void editor_world_controller_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		if (!_command_listener.is_null())
		{
			editor_command_system_t::get().remove_listener(_command_listener);
			_command_listener = {};
		}

		destroy_worlds_internal(false);
		_edit_contexts.clear();
		_main_edit_context = {};
		_previous_time_us  = 0;
		_accumulator_us	   = 0;
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		reset_camera_input();
		_main_camera_entity = NULL_ENTITY_ID;
		s_instance			= nullptr;
	}

	world_handle_t editor_world_controller_t::create_world(vec2u16_t render_resolution)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const world_handle_t handle = _worlds.add();
		world_t&			 world	= _worlds.get(handle);
		world.init();

		world_container_t& container = _world_containers.emplace_back();
		container.handle			 = handle;
		container.edit_context		 = _edit_contexts.emplace();

		editor_world_edit_context_t& context = _edit_contexts.get(container.edit_context);
		context.init();
		context.set_handle(container.edit_context);
		context.set_world(handle);
		container.producer_slot = 0;
		container.consumer_slot = 1;
		container.snapshot_mailbox.store(2, std::memory_order_relaxed);
		container.render_context.init(render_resolution);

		for (u32 i = 0; i < 3; i++)
		{
			container.snapshot_slots[i].reserve(8000);
		}

		return handle;
	}

	void editor_world_controller_t::destroy_world(world_handle_t handle)
	{
		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const bool was_main_world = _main_world == handle;
		if (was_main_world)
			set_main_world({}, NULL_SID, "");

		destroy_world_internal(handle);

		if (was_main_world)
		{
			_main_camera_entity = NULL_ENTITY_ID;
			reset_camera_input();
		}
	}

	void editor_world_controller_t::destroy_world_internal(world_handle_t handle)
	{
		for (auto it = _world_containers.begin(); it != _world_containers.end(); ++it)
		{
			if (it->handle == handle)
			{
				it->render_context.uninit();
				_edit_contexts.get(it->edit_context).uninit();
				_edit_contexts.remove(it->edit_context);
				world_t& world = _worlds.get(handle);
				world.unload_all_used_resources();
				world.uninit();
				_worlds.remove(handle);
				_world_containers.erase(it);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_world_controller_t::destroy_main_world_internal()
	{
		if (_main_world.is_null())
			return;

		const world_handle_t handle = _main_world;
		set_main_world({}, NULL_SID, "");
		destroy_world_internal(handle);
		_main_camera_entity = NULL_ENTITY_ID;
		reset_camera_input();
	}

	void editor_world_controller_t::destroy_worlds()
	{
		destroy_worlds_internal(true);
	}

	void editor_world_controller_t::destroy_worlds_internal(bool notify_panels)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		if (notify_panels && !_main_world.is_null())
			set_main_world({}, NULL_SID, "");

		for (world_container_t& container : _world_containers)
		{
			container.render_context.uninit();
			_edit_contexts.get(container.edit_context).uninit();
			_edit_contexts.remove(container.edit_context);
			world_t& world = _worlds.get(container.handle);
			world.unload_all_used_resources();
			world.uninit();
			_worlds.remove(container.handle);
		}

		_world_containers.resize(0);
		_main_camera_entity = NULL_ENTITY_ID;
		reset_camera_input();
		_main_world					   = {};
		_main_edit_context			   = {};
		_main_world_asset_guid		   = NULL_SID;
		_pending_main_world_asset_guid = NULL_SID;
		_main_world_name.resize(0);
		_main_world_dirty = false;
	}

	void editor_world_controller_t::resize_world(world_handle_t handle, vec2u16_t render_resolution)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
			{
				container.render_context.resize(render_resolution);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	bool editor_world_controller_t::render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);

		if (_world_containers.empty())
			return false;

		frame_vector_t<gfx_handle_t> command_buffers;
		command_buffers.reserve(_world_containers.size());

		const f32 interpolation_alpha = calculate_render_alpha();
		for (world_container_t& container : _world_containers)
		{
			const world_render_snapshot_t& snapshot = acquire_render_snapshot(container);
			world_rendering_t::render_world(container.render_context, snapshot, interpolation_alpha, frame_index, global_cbv_index, global_layout);
			command_buffers.push_back(container.render_context.get_command_buffer(frame_index));
		}

		gfx_backend& backend = gfx_backend::get();

		if (!_world_containers.empty())
			backend.queue_signal(queue, &signal, &signal_value, 1);
		return true;
	}

	void editor_world_controller_t::tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps)
	{
		_world_physics_rate = world_physics_rate;

		const i64 now	   = time_t::get_cpu_microseconds();
		const i64 delta_us = now - _previous_time_us;
		_previous_time_us  = now;

		u32		  steps	   = 0;
		const i64 fixed_us = world_tick_rate == 0 ? 0 : 1000000 / static_cast<i64>(world_tick_rate);
		if (fixed_us > 0)
		{
			_accumulator_us += delta_us;

			const f32 dt_seconds = 1.0f / static_cast<f32>(world_tick_rate);
			while (_accumulator_us >= fixed_us && steps < max_sim_steps)
			{
				_accumulator_us -= fixed_us;
				for (const world_container_t& container : _world_containers)
				{
					world_t& world = _worlds.get(container.handle);
					if (container.handle == _main_world)
						tick_editor_camera(dt_seconds);
					world.tick(dt_seconds);
					world.update_world_transforms(true);
				}
				++steps;
			}

			_fixed_step_us.store(fixed_us, std::memory_order_relaxed);
			_last_fixed_step_us.store(now - _accumulator_us, std::memory_order_release);
		}
		else
		{
			_accumulator_us = 0;
			_fixed_step_us.store(0, std::memory_order_relaxed);
			_last_fixed_step_us.store(now, std::memory_order_release);
		}

		for (world_container_t& container : _world_containers)
		{
			world_t& world = _worlds.get(container.handle);
			if (steps == 0)
				world.update_world_transforms(false);
			world_snapshot_producer_t::produce(world, container.snapshot_slots[container.producer_slot]);
			publish_world_snapshot(container);
		}
	}

	void editor_world_controller_t::install_default_world(world_handle_t handle)
	{
		world_t& world = _worlds.get(handle);
		install_editor_camera(world);

		const entity_id_t	environment				  = world.create_entity("environment");
		component_skybox_t& skybox					  = ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(type_id_t<component_skybox_t>::value)->table, environment);
		skybox.skybox_asset							  = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		skybox.exposure								  = 0.25f;
		world_component_table_t& debug_widgets_table  = *world.get_component_table(type_id_t<component_debug_widgets_t>::value);
		const bool				 debug_widgets_exists = ecs_t::table_has(debug_widgets_table.table, environment);
		void*					 debug_widgets		  = ecs_t::table_add(debug_widgets_table.table, environment);
		if (!debug_widgets_exists)
			debug_widgets_table.type_desc.default_init(debug_widgets);

		for (world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
			{
				world.add_resource(resource_type_e::hdr_skybox, DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID);
				world.load_all_used_resources();
				return;
			}
		}

		SFG_ASSERT(false);
	}

	bool editor_world_controller_t::load_main_world(sid_t asset_guid)
	{
		editor_app_t::get().stop_render();

		if (!_main_world.is_null() && _main_world_dirty)
		{
			_pending_main_world_asset_guid		 = asset_guid;
			editor_modal_button_desc_t buttons[] = {
				{.text = "Save", .callback = on_save_dirty_world_modal, .user_data = this},
				{.text = "Don't Save", .callback = on_dont_save_dirty_world_modal, .user_data = this},
				{.text = "Cancel", .callback = on_cancel_dirty_world_modal, .user_data = this},
			};
			editor_app_t::get().get_main_surface().modal_controller->request_modal("Save World", "Would you like to save the current world.", buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), editor_modal_severity_e::warning);
			return true;
		}

		return load_main_world_now(asset_guid);
	}

	void editor_world_controller_t::load_dummy_world()
	{
		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_main_world_internal();

		const world_handle_t handle = create_world(editor_app_t::get().get_main_surface().swapchain_size);
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

		const world_handle_t handle = create_world(editor_app_t::get().get_main_surface().swapchain_size);
		if (asset->embedded_source.empty())
			install_default_world(handle);
		else
		{
			world_t&			 world			 = _worlds.get(handle);
			const nlohmann::json embedded_source = editor_asset_util_t::get_embedded_source_json(*asset);
			world_cooker_t::world_from_json(world, embedded_source);
			world.load_all_used_resources();
			install_editor_camera(world);
		}

		const char* asset_name = editor_asset_util_t::find_asset_display_name(asset_guid);
		set_main_world(handle, asset_guid, asset_name != nullptr ? asset_name : "unnamed");
		if (!asset->embedded_source.empty())
		{
			const nlohmann::json embedded_source = editor_asset_util_t::get_embedded_source_json(*asset);
			get_main_edit_context().read_folders_from_json(embedded_source.value<nlohmann::json>("folders", nlohmann::json::array()));
			if (editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities))
				static_cast<editor_panel_entities_t*>(panel)->refresh_entities();
		}
		_main_world_dirty		  = false;
		editor_project_t& project = editor_project_t::get();
		project.last_world_guid	  = asset_guid;
		project.save(project._runtime.path.c_str());
		return true;
	}

	bool editor_world_controller_t::save_main_world()
	{
		if (_main_world.is_null())
			return false;

		nlohmann::json world_json = nlohmann::json::object();
		world_cooker_t::world_to_json(_worlds.get(_main_world), world_json);
		get_main_edit_context().write_folders_to_json(world_json["folders"]);

		editor_asset_t		  asset = {};
		string_t			  asset_path;
		const editor_asset_t* existing_asset = _main_world_asset_guid != NULL_SID ? editor_asset_manager_t::get().find_asset(_main_world_asset_guid) : nullptr;
		if (existing_asset != nullptr && existing_asset->asset_type == editor_asset_type_e::world)
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			for (auto it = tree.begin_handle(); it != tree.end_handle(); ++it)
			{
				const editor_asset_node_t& node = tree.value(*it);
				if (node.type == editor_asset_node_type_e::asset && node.asset_id == _main_world_asset_guid)
				{
					asset_path = node.full_path;
					break;
				}
			}

			if (!asset_path.empty() && file_system_t::exists(asset_path.c_str()))
			{
				if (!editor_asset_util_t::read_asset(asset_path.c_str(), asset))
					return false;
			}
		}

		if (asset_path.empty() || !file_system_t::exists(asset_path.c_str()))
		{
			asset_path = process::save_file("Save World", "sfg_asset");
			if (asset_path.empty())
			{
				SFG_ERR("world save cancelled");
				return false;
			}
			if (file_system_t::get_file_extension(asset_path) != "sfg_asset")
				asset_path += ".sfg_asset";
			if (!editor_asset_util_t::is_source_inside_assets(editor_project_t::get()._runtime.assets_path.c_str(), asset_path.c_str()))
			{
				SFG_ERR("world save path is outside project assets directory {0}", asset_path.c_str());
				return false;
			}

			asset.version	  = editor_asset_t::VERSION;
			asset.guid		  = _main_world_asset_guid != NULL_SID ? _main_world_asset_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type  = editor_asset_type_e::world;
			asset.source_type = editor_asset_source_type_e::embedded;
		}

		asset.asset_type  = editor_asset_type_e::world;
		asset.source_type = editor_asset_source_type_e::embedded;
		editor_asset_util_t::set_embedded_source_json(asset, world_json);
		if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to save world asset {0}", asset_path.c_str());
			return false;
		}

		_main_world_asset_guid	  = asset.guid;
		_main_world_dirty		  = false;
		editor_project_t& project = editor_project_t::get();
		project.last_world_guid	  = asset.guid;
		project.save(project._runtime.path.c_str());
		editor_asset_manager_t::get().rescan(editor_project_t::get()._runtime.assets_path);
		return true;
	}

	void editor_world_controller_t::set_main_world(world_handle_t handle, sid_t asset_guid, const char* name)
	{
		SFG_ASSERT(name != nullptr);

		if (_main_world == handle && _main_world_asset_guid == asset_guid && _main_world_name == name)
			return;

		_main_world					   = handle;
		_main_edit_context			   = handle.is_null() ? editor_world_edit_context_handle_t{} : get_edit_context(handle).get_handle();
		_main_world_asset_guid		   = asset_guid;
		_pending_main_world_asset_guid = NULL_SID;
		_main_world_name			   = name;
		_main_world_dirty			   = false;
		editor_command_system_t::get().clear();
		notify_main_world_changed();
	}

	void editor_world_controller_t::set_main_world_dirty()
	{
		if (!_main_world.is_null())
			_main_world_dirty = true;
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
			controller.set_main_world_dirty();
			break;
		default:
			break;
		}
	}

	void editor_world_controller_t::notify_main_world_changed()
	{
		editor_app_t& app = editor_app_t::get();

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::world))
		{
			editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
			if (_main_world.is_null())
				world_panel->clear_world();
			else
				world_panel->set_world(get_world_render_context(_main_world), _main_world_name.c_str());
		}

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::entities))
		{
			editor_panel_entities_t* entities_panel = static_cast<editor_panel_entities_t*>(panel);
			entities_panel->set_edit_context(_main_edit_context);
			entities_panel->refresh_entities();
		}

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::inspector))
		{
			editor_panel_inspector_t* inspector_panel = static_cast<editor_panel_inspector_t*>(panel);
			inspector_panel->set_edit_context(_main_edit_context);
			inspector_panel->refresh_from_selection();
		}
	}

	void editor_world_controller_t::reset_input(window_runtime_t& runtime)
	{
		if (_is_looking)
		{
			process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
			process::set_cursor_visible(true);
		}
		reset_camera_input();
	}

	bool editor_world_controller_t::on_window_event(surface_handle_t surface_handle, window_runtime_t& runtime, const window_event_t& ev)
	{
		if (ev.type == window_event_type_e::focus && ev.sub_type == window_event_sub_type_e::release)
		{
			if (_is_looking)
			{
				process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
				process::set_cursor_visible(true);
			}
			reset_camera_input();
			return false;
		}

		editor_panel_t* panel = editor_app_t::get().find_panel_on_surface(editor_panel_type_e::world, surface_handle);
		if (panel == nullptr || !panel->is_inited())
			return false;

		editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
		const vec4f_t		  bounds	  = world_panel->get_world_view_bounds();
		const vec2f_t		  mouse		  = {static_cast<f32>(runtime.mouse_position.x), static_cast<f32>(runtime.mouse_position.y)};
		const bool			  inside	  = mouse.x >= bounds.x && mouse.y >= bounds.y && mouse.x <= bounds.x + bounds.z && mouse.y <= bounds.y + bounds.w;

		switch (ev.type)
		{
		case window_event_type_e::mouse:
			if (ev.sub_type == window_event_sub_type_e::press)
			{
				if (!inside)
				{
					if (_is_looking)
					{
						process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
						process::set_cursor_visible(true);
					}
					reset_camera_input();
					return false;
				}

				_world_panel_focused = true;
				if (ev.button == static_cast<u16>(input_code::mouse_right))
				{
					_is_looking = true;
					process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::pointer);
					process::set_cursor_visible(false);
				}
				return true;
			}

			if (ev.sub_type == window_event_sub_type_e::release && ev.button == static_cast<u16>(input_code::mouse_right) && _is_looking)
			{
				_is_looking	 = false;
				_mouse_delta = vec2f_t::zero;
				process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
				process::set_cursor_visible(true);
				return true;
			}
			break;
		case window_event_type_e::delta:
			if (_is_looking)
			{
				_mouse_delta.x += static_cast<f32>(ev.value.x);
				_mouse_delta.y += static_cast<f32>(ev.value.y);
				return true;
			}
			break;
		case window_event_type_e::key:
			if (!_world_panel_focused)
				return false;

			if (ev.button == static_cast<u16>(input_code::key_w) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.z += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_w) && ev.sub_type == window_event_sub_type_e::release && _direction_input.z > 0.1f)
				_direction_input.z -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.z -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::release && _direction_input.z < -0.1f)
				_direction_input.z += 1.0f;

			if (ev.button == static_cast<u16>(input_code::key_d) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.x += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_d) && ev.sub_type == window_event_sub_type_e::release && _direction_input.x > 0.1f)
				_direction_input.x -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_a) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.x -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_a) && ev.sub_type == window_event_sub_type_e::release && _direction_input.x < -0.1f)
				_direction_input.x += 1.0f;

			if (ev.button == static_cast<u16>(input_code::key_e) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.y += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_e) && ev.sub_type == window_event_sub_type_e::release && _direction_input.y > 0.1f)
				_direction_input.y -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_q) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.y -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_q) && ev.sub_type == window_event_sub_type_e::release && _direction_input.y < -0.1f)
				_direction_input.y += 1.0f;

			if ((ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift)) && ev.sub_type == window_event_sub_type_e::press)
				_current_move_speed = EDITOR_CAMERA_BASE_MOVE_SPEED * EDITOR_CAMERA_BOOST_MULTIPLIER;
			else if ((ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift)) && ev.sub_type == window_event_sub_type_e::release)
				_current_move_speed = EDITOR_CAMERA_BASE_MOVE_SPEED;

			if (ev.button == static_cast<u16>(input_code::key_a) || ev.button == static_cast<u16>(input_code::key_d) || ev.button == static_cast<u16>(input_code::key_w) || ev.button == static_cast<u16>(input_code::key_s) ||
				ev.button == static_cast<u16>(input_code::key_q) || ev.button == static_cast<u16>(input_code::key_e) || ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift))
				return true;
			break;
		default:
			break;
		}

		return false;
	}

	const world_render_context_t& editor_world_controller_t::get_world_render_context(world_handle_t handle) const
	{
		for (const world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
				return container.render_context;
		}

		SFG_ASSERT(false);
		return _world_containers.front().render_context;
	}

	editor_world_edit_context_t& editor_world_controller_t::get_edit_context(world_handle_t handle)
	{
		for (world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
				return _edit_contexts.get(container.edit_context);
		}

		SFG_ASSERT(false);
		return _edit_contexts.get(_main_edit_context);
	}

	const editor_world_edit_context_t& editor_world_controller_t::get_edit_context(world_handle_t handle) const
	{
		for (const world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
				return _edit_contexts.get(container.edit_context);
		}

		SFG_ASSERT(false);
		return _edit_contexts.get(_main_edit_context);
	}

	void editor_world_controller_t::publish_world_snapshot(world_container_t& container)
	{
		const u8 prev			= container.snapshot_mailbox.exchange(container.producer_slot | WORLD_SNAPSHOT_FRESH_FLAG, std::memory_order_release);
		container.producer_slot = static_cast<u8>((prev & WORLD_SNAPSHOT_SLOT_MASK) % WORLD_SNAPSHOT_SLOT_COUNT);
	}

	const world_render_snapshot_t& editor_world_controller_t::acquire_render_snapshot(world_container_t& container)
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 cur = container.snapshot_mailbox.load(std::memory_order_acquire);
		while (cur & WORLD_SNAPSHOT_FRESH_FLAG)
		{
			if (container.snapshot_mailbox.compare_exchange_weak(cur, container.consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				container.consumer_slot = static_cast<u8>((cur & WORLD_SNAPSHOT_SLOT_MASK) % WORLD_SNAPSHOT_SLOT_COUNT);
				break;
			}
		}
		return container.snapshot_slots[container.consumer_slot];
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

	void editor_world_controller_t::install_editor_camera(world_t& world)
	{
		const entity_id_t	camera_entity = world.create_entity("editor camera");
		component_camera_t& camera		  = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value)->table, camera_entity);
		camera.priority					  = -1;
		ecs_t::table_add(world.get_component_table(type_id_t<component_no_serialize_t>::value)->table, camera_entity);
		ecs_t::table_add(world.get_component_table(type_id_t<component_render_object_t>::value)->table, camera_entity);
		_main_camera_entity	  = camera_entity;
		const vec3f_t euler	  = quat_t::to_euler(world.get_entity_rot_local(camera_entity));
		_camera_pitch_degrees = euler.x;
		_camera_yaw_degrees	  = euler.y;
	}

	void editor_world_controller_t::reset_camera_input()
	{
		_direction_input	 = vec3f_t::zero;
		_mouse_delta		 = vec2f_t::zero;
		_current_move_speed	 = EDITOR_CAMERA_BASE_MOVE_SPEED;
		_world_panel_focused = false;
		_is_looking			 = false;
	}

	void editor_world_controller_t::tick_editor_camera(f32 dt_seconds)
	{
		if (_main_camera_entity == NULL_ENTITY_ID || !_world_panel_focused)
			return;

		world_t& world = _worlds.get(_main_world);

		_camera_yaw_degrees -= _mouse_delta.x * EDITOR_CAMERA_MOUSE_SENSITIVITY;
		_camera_pitch_degrees -= _mouse_delta.y * EDITOR_CAMERA_MOUSE_SENSITIVITY;
		_camera_pitch_degrees = math::clamp(_camera_pitch_degrees, -89.0f, 89.0f);
		_mouse_delta		  = vec2f_t::zero;

		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		world.set_entity_rot_local(_main_camera_entity, rotation);

		vec3f_t move_dir = rotation.get_forward() * _direction_input.z + rotation.get_right() * _direction_input.x + vec3f_t::up * _direction_input.y;
		if (move_dir.is_zero())
			return;

		move_dir.normalize();
		const vec3f_t position = world.get_entity_pos_local(_main_camera_entity) + move_dir * (_current_move_speed * dt_seconds);
		world.set_entity_pos_local(_main_camera_entity, position);
	}
}
