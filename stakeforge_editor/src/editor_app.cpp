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
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include "editor_project.hpp"
#include "editor_project.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/panels/editor_panel_factory.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include "ui/panels/editor_secondary_base.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		vec2u16_t get_layout_window_size(const editor_layout_window_t& window)
		{
			return (window.size.x == 0 || window.size.y == 0) ? vec2u16_t{1920, 1080} : window.size;
		}

		ui::mouse_button_e map_button(u16 b)
		{
			if (b == static_cast<u16>(input_code::mouse_right))
				return ui::mouse_button_e::right;
			if (b == static_cast<u16>(input_code::mouse_middle))
				return ui::mouse_button_e::middle;
			return ui::mouse_button_e::left;
		}

		editor_panel_t* find_panel_in_surface(editor_surface_t& surface, editor_panel_type_e type)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface.primary->get_dock_widget().find_panel(type);
			if (surface.type == editor_surface_type_e::secondary)
				return surface.secondary->get_dock_widget().find_panel(type);
			return nullptr;
		}

		bool select_panel_in_surface(editor_surface_t& surface, editor_panel_t* panel)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface.primary->get_dock_widget().select_panel(panel);
			if (surface.type == editor_surface_type_e::secondary)
				return surface.secondary->get_dock_widget().select_panel(panel);
			return false;
		}

	}

	void editor_app_t::on_window_event(void*, const window_event_t& ev, void* user_data)
	{
		window_runtime_t& runtime = *static_cast<window_runtime_t*>(user_data);
		editor_surface_t& surface = editor_app_t::get().get_surface_by_runtime(runtime);
		ui::ui_context&	  ui	  = *surface.ui;

		switch (ev.type)
		{
		case window_event_type_e::delta:
		case window_event_type_e::mouse: {
			const vec2i16_t mp = runtime.mouse_position;
			ui.on_mouse_move({static_cast<f32>(mp.x), static_cast<f32>(mp.y)});

			if (ev.type == window_event_type_e::mouse)
			{
				if (ev.sub_type == window_event_sub_type_e::press)
					ui.on_mouse_button(map_button(ev.button), true);
				else if (ev.sub_type == window_event_sub_type_e::release)
					ui.on_mouse_button(map_button(ev.button), false);
			}
			break;
		}
		case window_event_type_e::wheel:
			ui.on_wheel(static_cast<f32>(ev.value.y));
			break;
		case window_event_type_e::key: {
			if (ev.button == static_cast<u16>(input_code::key_f3) && ev.sub_type == window_event_sub_type_e::press)
				editor_app_t::get().set_debug_mode(!editor_app_t::get()._debug_mode);
			if (ev.button == static_cast<u16>(input_code::key_f4) && ev.sub_type == window_event_sub_type_e::press)
				editor_app_t::get().create_payload("Debug payload", editor_payload_type_e::resource, nullptr);
			if (ev.button == static_cast<u16>(input_code::key_f5) && ev.sub_type == window_event_sub_type_e::press)
			{
				editor_app_t& app		  = editor_app_t::get();
				app._debug_modal_progress = 0.0f;
				app._debug_progress_modal.set_progress(app._debug_modal_progress);
				editor_modal_content_desc_t progress_content = app._debug_progress_modal.get_content_desc();
				app.get_main_surface().modal_controller->request_modal("Debug Loading", "Debug modal progress.", false, nullptr, 0, &progress_content);
			}
			if (ev.button == static_cast<u16>(input_code::key_f6) && ev.sub_type == window_event_sub_type_e::press)
			{
				editor_app_t& app		  = editor_app_t::get();
				app._debug_modal_progress = math::min(app._debug_modal_progress + 0.1f, 1.0f);
				app._debug_progress_modal.set_progress(app._debug_modal_progress);
			}
			if (ev.button == static_cast<u16>(input_code::key_f7) && ev.sub_type == window_event_sub_type_e::press)
			{
				editor_app_t& app		  = editor_app_t::get();
				app._debug_modal_progress = 0.0f;
				app.get_main_surface().modal_controller->close_modal();
			}

			ui::key_event_t k = {};
			k.key			  = ev.button;
			k.scan_code		  = static_cast<u16>(ev.value.x);
			k.action		  = ev.sub_type == window_event_sub_type_e::press ? ui::key_action_e::press : (ev.sub_type == window_event_sub_type_e::release ? ui::key_action_e::release : ui::key_action_e::repeat);
			ui.on_key(k);
			break;
		}
		default:
			break;
		}
	}

	bool editor_app_t::on_window_client_hit_test(window_runtime_t& runtime, const vec2i16_t& pos, void* user_data)
	{
		editor_app_t&	  app	  = *static_cast<editor_app_t*>(user_data);
		editor_surface_t& surface = app.get_surface_by_runtime(runtime);
		if (surface.type == editor_surface_type_e::primary)
			return surface.primary->is_window_drag_region(pos);
		if (surface.type == editor_surface_type_e::secondary)
			return surface.secondary->is_window_drag_region(pos);
		return false;
	}

	bool editor_app_t::init()
	{
		editor_text_rasterization_t::set_subpixel_enabled(true);
		editor_directories_t::init_paths();
		if (!editor_settings_t::get().ensure_loaded())
			return false;

		/* engine globals, init backend & engine & editor managers */
		engine_runtime_t::init_globals(64ull * 1024ull * 1024ull);

		if (!engine_runtime_t::init_backend({}))
		{
			engine_runtime_t::uninit_globals();
			return false;
		}

		if (!_runtime.init())
		{
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		_world_controller.init(_runtime);
		_asset_manager.init();

		/* resources & renderers init */

		resource_pack_t::init_params_t pack_params;
		pack_params.manifest_path = editor_directories_t::get_editor_manifest();
		pack_params.assets_dir	  = editor_directories_t::get_editor_assets();
		pack_params.cache_dir	  = editor_directories_t::get_editor_resource_cache();
		if (!_resource_pack.init(resource_manager_t::get(), pack_params))
		{
			_world_controller.uninit();
			_runtime.uninit();
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}
		resource_manager_t::get().wait_for_all_complete();

		if (!_renderer.init())
		{
			_resource_pack.uninit();
			_world_controller.uninit();
			_runtime.uninit();
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		frame_allocator_tls_t::init(MAIN_FRAME_ALLOC_SIZE);

		/* payload window & layout/surface initing */

		const surface_handle_t payload_surface = create_surface({0, 0}, {160, 24}, editor_surface_type_e::payload);
		if (payload_surface.is_null())
			return false;
		_payload_controller.init(_surfaces.get(payload_surface));
		_payload_controller.set_unhandled_listener(on_payload_unhandled, this);

		const editor_layout_t& layout = editor_settings_t::get().layout;
		if (layout.windows.empty())
		{
			const editor_layout_window_t window = {};
			const surface_handle_t		 handle = create_surface(window.pos, get_layout_window_size(window), editor_surface_type_e::primary);
			if (handle.is_null())
				return false;
			process::set_window_maximized(_surfaces.get(handle).runtime->window_handle, window.maximized);
			editor_layout_t::load_surface_default_layout(_surfaces.get(handle));
		}
		else
		{
			const editor_layout_window_t* primary_window = nullptr;
			for (const editor_layout_window_t& window : layout.windows)
			{
				if (window.is_primary)
				{
					primary_window = &window;
					break;
				}
			}

			SFG_ASSERT(primary_window != nullptr);
			if (primary_window == nullptr)
				return false;

			const surface_handle_t primary_handle = create_surface(primary_window->pos, get_layout_window_size(*primary_window), editor_surface_type_e::primary);
			process::set_window_maximized(_surfaces.get(primary_handle).runtime->window_handle, primary_window->maximized);
			load_surface_dock_layout(_surfaces.get(primary_handle), primary_window->dock_layout);

			for (const editor_layout_window_t& window : layout.windows)
			{
				if (&window == primary_window)
					continue;

				const surface_handle_t secondary_handle = create_surface(window.pos, get_layout_window_size(window), editor_surface_type_e::secondary);
				process::set_window_maximized(_surfaces.get(secondary_handle).runtime->window_handle, window.maximized);
				load_surface_dock_layout(_surfaces.get(secondary_handle), window.dock_layout);
			}
		}

		if (_surfaces.empty())
		{
			_renderer.uninit();
			_resource_pack.uninit();
			_world_controller.uninit();
			_runtime.uninit();
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		_last_tick_us = time_t::get_cpu_microseconds();

		const string_t& last_project = editor_settings_t::get().last_project_path;
		if (!file_system_t::exists(last_project.c_str()) || !load_project(last_project.c_str()))
		{
			get_main_surface().primary->prompt_no_project_modal();
		}

		const world_handle_t main_world = _world_controller.create_world(get_main_surface().swapchain_size);
		_world_controller.set_main_world(main_world);
		set_main_world_to_panel();

		_close = false;

		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		SFG_ASSERT(_surfaces.empty());

		_renderer.end_render();
		resource_manager_t::get().flush();
		_renderer.uninit();
		_resource_pack.uninit();
		_asset_manager.uninit();
		_surfaces.resize_zero();
		_world_controller.uninit();
		_runtime.uninit();
		engine_runtime_t::uninit_globals();
		engine_runtime_t::uninit_backend();
		frame_allocator_tls_t::uninit();
	}

	void editor_app_t::load_surface_dock_layout(editor_surface_t& surface, const string_t& dock_layout)
	{
		const nlohmann::json doc = nlohmann::json::parse(dock_layout, nullptr, false);
		if (doc.is_discarded() || !doc.is_object())
			return;

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->get_dock_widget().from_json(doc);
		else if (surface.type == editor_surface_type_e::secondary)
			surface.secondary->get_dock_widget().from_json(doc);
	}

	void editor_app_t::set_debug_mode(bool enabled)
	{
		_debug_mode = enabled;
		for (editor_surface_t& surface : _surfaces)
			surface.ui->set_debug_draw(enabled);
	}

	void editor_app_t::set_text_subpixel_enabled(bool enabled)
	{
		editor_text_rasterization_t::set_subpixel_enabled(enabled);
		const ui::glyph_raster_mode_e raster_mode = editor_text_rasterization_t::get_rasterization_type();
		for (editor_surface_t& surface : _surfaces)
			surface.ui->get_paint().set_text_raster_mode(raster_mode);
	}

	void editor_app_t::create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value)
	{
		_payload_controller.create_payload(text, type, user_ptr, size_value);
	}

	void editor_app_t::on_payload_unhandled(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::panel)
			return;

		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_app_t&	app	  = *static_cast<editor_app_t*>(user_data);
		editor_panel_t* panel = static_cast<editor_panel_t*>(payload.user_ptr);
		vec2u16_t		size  = payload.size_value;
		if (size.x == 0 || size.y == 0)
			size = {640, 480};
		size.y = static_cast<u16>(size.y + editor_theme_t::get().item_height);

		const surface_handle_t surface_handle = app.create_surface(payload.pos, size, editor_surface_type_e::secondary);
		if (surface_handle.is_null())
			return;

		editor_surface_t&		 surface = app._surfaces.get(surface_handle);
		dock_widget_t&			 dock	 = surface.secondary->get_dock_widget();
		const dock_node_handle_t leaf	 = dock.create_leaf_node(dock.get_root());
		dock.set_root_node(leaf);
		dock.dock_node_add_panel(leaf, panel);
	}

	void editor_app_t::unload_current_project()
	{
		_renderer.end_render();
		_asset_manager.clear();
		_world_controller.destroy_worlds();
		set_main_world_to_panel();
	}

	bool editor_app_t::create_project(const char* path)
	{
		SFG_ASSERT(file_system_t::get_file_extension(path) == "sfg_project");

		const editor_project_t project	 = editor_project_t::make_default_project(path);
		const nlohmann::json   json_data = project;
		const string_t		   data		 = json_data.dump(4);
		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), path))
			return false;

		return load_project(path);
	}

	bool editor_app_t::load_project(const char* path)
	{
		SFG_ASSERT(file_system_t::get_file_extension(path) == "sfg_project");
		SFG_ASSERT(file_system_t::exists(path));

		unload_current_project();

		editor_project_t& proj = editor_project_t::get();
		if (!proj.try_load(path))
		{
			return false;
		}

		_asset_manager.ensure_default_assets(proj._runtime.default_assets_path.c_str());
		_asset_manager.ensure_cook();
		editor_settings_t::get().last_project_path = path;
		editor_settings_t::get().save();
		get_main_surface().primary->set_current_project_name(proj._runtime.name.c_str());
		return true;
	}

	void editor_app_t::save_layout()
	{
		editor_layout_t& layout = editor_settings_t::get().layout;
		layout					= {};

		bool primary_saved = false;
		for (const editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::payload)
				continue;

			editor_layout_window_t window = {};
			window.pos					  = surface.runtime->pos;
			window.size					  = surface.runtime->size;
			window.is_primary			  = surface.type == editor_surface_type_e::primary;
			window.maximized			  = surface.runtime->has_flag(window_runtime_flags_e::maximized);
			if (surface.type == editor_surface_type_e::primary)
				window.dock_layout = string_t(surface.primary->get_dock_widget().to_json().dump());
			else if (surface.type == editor_surface_type_e::secondary)
				window.dock_layout = string_t(surface.secondary->get_dock_widget().to_json().dump());
			if (window.is_primary)
			{
				SFG_ASSERT(!primary_saved);
				primary_saved = true;
			}
			layout.windows.push_back(window);
		}

		editor_settings_t::get().save();
	}

	void editor_app_t::apply_default_layout()
	{
		frame_vector_t<surface_handle_t> destroy_handles;
		for (u16 i = 0; i < _surfaces.head(); ++i)
		{
			if (!_surfaces.is_active(i))
				continue;

			const surface_handle_t	handle	= _surfaces.get_handle(i);
			const editor_surface_t& surface = _surfaces.get(handle);
			if (surface.type == editor_surface_type_e::secondary)
				destroy_handles.push_back(handle);
		}

		for (surface_handle_t handle : destroy_handles)
			destroy_surface(handle);

		editor_layout_t::load_surface_default_layout(get_main_surface());
		save_layout();
	}

	editor_panel_t* editor_app_t::find_panel(editor_panel_type_e type, surface_handle_t surface_handle)
	{
		if (!surface_handle.is_null())
		{
			editor_panel_t* panel = find_panel_in_surface(_surfaces.get(surface_handle), type);
			if (panel != nullptr)
				return panel;

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle = *it;
				if (handle == surface_handle)
					continue;

				panel = find_panel_in_surface(_surfaces.get(handle), type);
				if (panel != nullptr)
					return panel;
			}
			return nullptr;
		}

		surface_handle_t main_surface = {};
		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle  = *it;
			editor_surface_t&	   surface = _surfaces.get(handle);
			if (surface.type != editor_surface_type_e::primary)
				continue;

			main_surface		  = handle;
			editor_panel_t* panel = find_panel_in_surface(surface, type);
			if (panel != nullptr)
				return panel;
			break;
		}

		SFG_ASSERT(!main_surface.is_null());
		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle = *it;
			if (handle == main_surface)
				continue;

			editor_panel_t* panel = find_panel_in_surface(_surfaces.get(handle), type);
			if (panel != nullptr)
				return panel;
		}
		return nullptr;
	}

	void editor_app_t::show_panel(editor_panel_type_e type, surface_handle_t surface_handle)
	{
		editor_panel_t* panel = find_panel(type, surface_handle);
		if (panel != nullptr)
		{
			for (editor_surface_t& surface : _surfaces)
			{
				if (!select_panel_in_surface(surface, panel))
					continue;

				process::bring_to_front(surface.runtime->window_handle);
				return;
			}
			SFG_ASSERT(false);
			return;
		}

		const editor_surface_t& main_surface = get_main_surface();
		const vec2i16_t			pos			 = {static_cast<i16>(main_surface.runtime->pos.x + 64), static_cast<i16>(main_surface.runtime->pos.y + 64)};
		const vec2u16_t			size		 = {640, 480};
		const surface_handle_t	new_surface	 = create_surface(pos, size, editor_surface_type_e::secondary);
		if (new_surface.is_null())
			return;

		panel							 = editor_panel_factory_t::create_panel(type);
		editor_surface_t&		 surface = _surfaces.get(new_surface);
		dock_widget_t&			 dock	 = surface.secondary->get_dock_widget();
		const dock_node_handle_t leaf	 = dock.create_leaf_node(dock.get_root());
		dock.set_root_node(leaf);
		dock.dock_node_add_panel(leaf, panel);
		process::bring_to_front(surface.runtime->window_handle);
	}

	void editor_app_t::set_main_world_to_panel()
	{
		editor_panel_t* panel = find_panel(editor_panel_type_e::world);
		if (panel == nullptr)
			return;

		editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
		const world_handle_t  main_world  = _world_controller.get_main_world();
		if (main_world.is_null())
		{
			world_panel->clear_world();
			return;
		}

		world_panel->set_world(_world_controller.get_world_render_context(main_world));
	}

	editor_surface_t& editor_app_t::get_main_surface()
	{
		SFG_ASSERT(!_surfaces.empty());
		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface;
		}
		SFG_ASSERT(false);
		return *_surfaces.begin();
	}

	editor_surface_t& editor_app_t::get_surface_by_runtime(window_runtime_t& runtime)
	{
		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.runtime.get() == &runtime)
				return surface;
		}
		SFG_ASSERT(false);
		return *_surfaces.begin();
	}

	void editor_app_t::tick()
	{
		bool tick = true;

		while (tick)
		{
			editor_project_t& project = editor_project_t::get();

			frame_allocator_tls_t::reset();

			process::pump_os_messages();

			_resource_pack.tick();
			resource_manager_t::get().flush();
			_asset_manager.tick();

			const i64 now = time_t::get_cpu_microseconds();
			const f32 dt  = static_cast<f32>(now - _last_tick_us) / 1.0e6f;
			_last_tick_us = now;

			_payload_controller.tick();
			_world_controller.tick(project.world_tick_rate, project.world_physics_rate, project.max_sim_steps);

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime->has_flag(window_runtime_flags_e::close_requested) || _close)
				{
					_renderer.end_render();
					if (surface.type == editor_surface_type_e::primary)
						_close = true;

					destroy_surface(handle);

					continue;
				}

				const bool minimized = surface.runtime->has_flag(window_runtime_flags_e::minimized);
				const bool hidden	 = surface.runtime->is_hidden;

				if (minimized != surface.is_minimized)
				{
					surface.is_minimized = minimized;
					_renderer.end_render();
					_renderer.set_swapchain_minimized(surface.swapchain, minimized);
				}

				if (hidden != surface.is_hidden)
				{
					surface.is_hidden = hidden;
					_renderer.end_render();
					_renderer.set_swapchain_visible(surface.swapchain, !hidden);
				}

				if (!minimized && !hidden && surface.runtime->size != surface.swapchain_size)
				{
					_renderer.end_render();
					_renderer.resize_swapchain(surface.swapchain, surface.runtime->size, surface.runtime->monitor_info.dpi_scale);
					surface.swapchain_size = surface.runtime->size;
				}

				if (!minimized && !hidden)
				{
					const vec4f_t screen	= {0.0f, 0.0f, static_cast<f32>(surface.swapchain_size.x), static_cast<f32>(surface.swapchain_size.y)};
					const f32	  dpi_scale = surface.runtime->monitor_info.dpi_scale > 0.0f ? surface.runtime->monitor_info.dpi_scale : 1.0f;
					surface.ui->tick(screen, dpi_scale, dt);
					surface.ui->publish_frame();
				}
			}

			resource_manager_t::get().drain_atlases(_atlas_upload_frame_slot);
			_atlas_upload_frame_slot = static_cast<u8>((_atlas_upload_frame_slot + 1) % BACK_BUFFER_COUNT);

			if (_surfaces.empty())
			{
				tick = false;
				break;
			}

			_renderer.ensure_render(_world_controller);
		}

		_renderer.end_render();
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size, editor_surface_type_e type)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_renderer.end_render();

		if (size.x == 0 || size.y == 0)
		{
			SFG_ERR("can't create a surface with zero size.");
			return {};
		}

		const surface_handle_t handle  = _surfaces.add();
		editor_surface_t&	   surface = _surfaces.get(handle);
		surface.type				   = type;
		surface.runtime				   = make_unique<window_runtime_t>();

		const window_style_e window_style = type == editor_surface_type_e::payload ? window_style_e::alpha : window_style_e::borderless;
		if (!process::create_window("Stakeforge Editor", pos, size, window_style, 0.75f, type == editor_surface_type_e::payload, *surface.runtime))
		{
			SFG_ERR("failed creating editor surface window!");
			_surfaces.remove(handle);
			return {};
		}

		surface.ui = make_unique<ui::ui_context>();
		surface.ui->init({
			.canvas =
				{
					.vertex_buffer_bytes = 1 << 23,
					.index_buffer_bytes	 = 1 << 23,
					.buffer_count		 = 32,
				},
			.user_ui_scale = 1.0f,
			.dpi_scale	   = surface.runtime->monitor_info.dpi_scale,
			.max_widgets   = 4096,
		});
		surface.ui->get_paint().set_pipelines({
			.default_pipeline		 = "editor/shaders/editor_ui_default.hlsl"_hs,
			.text_pipeline			 = "editor/shaders/editor_ui_text_lcd.hlsl"_hs,
			.grayscale_text_pipeline = "editor/shaders/editor_ui_text_grayscale.hlsl"_hs,
			.sdf_pipeline			 = "editor/shaders/editor_ui_sdf.hlsl"_hs,
		});
		surface.ui->set_debug_draw(_debug_mode);

		surface.tooltip_controller = make_unique<editor_tooltip_controller_t>();
		surface.tooltip_controller->init(*surface.ui);

		surface.modal_controller = make_unique<editor_modal_controller_t>();
		surface.modal_controller->init(*surface.ui);

		surface.popup_controller = make_unique<editor_popup_controller_t>();
		surface.popup_controller->init(*surface.ui);

		surface.action_menu_controller = make_unique<editor_action_menu_controller_t>();
		surface.action_menu_controller->init(*surface.ui);
		if (surface.type == editor_surface_type_e::primary)
		{
			surface.primary = make_unique<editor_primary_base_t>();
			surface.primary->init(*surface.ui, *surface.runtime);
		}
		else if (surface.type == editor_surface_type_e::secondary)
		{
			surface.secondary = make_unique<editor_secondary_base_t>();
			surface.secondary->init(*surface.ui, *surface.runtime);
		}

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime->window_handle, surface.runtime->platform_handle, surface.runtime->monitor_info.dpi_scale, surface.runtime->size, surface.ui.get());
		surface.swapchain_size = surface.runtime->size;

		surface.runtime->event_callback			   = &editor_app_t::on_window_event;
		surface.runtime->event_callback_user_data  = surface.runtime.get();
		surface.runtime->client_hit_test_callback  = &editor_app_t::on_window_client_hit_test;
		surface.runtime->client_hit_test_user_data = this;

		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_renderer.end_render();

		editor_surface_t& surface = _surfaces.get(handle);
		if (surface.type == editor_surface_type_e::primary)
			save_layout();

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->uninit();
		else if (surface.type == editor_surface_type_e::secondary)
			surface.secondary->uninit();
		else
			_payload_controller.uninit();

		surface.tooltip_controller->uninit();
		surface.popup_controller->uninit();
		surface.modal_controller->uninit();
		surface.action_menu_controller->uninit();
		surface.ui->uninit();
		surface.ui.reset();
		_renderer.destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime->window_handle);
		_surfaces.remove(handle);
	}

}
