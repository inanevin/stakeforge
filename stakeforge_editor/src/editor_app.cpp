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
#include "editor_modal_controller.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_panel.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

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

	}

	void editor_app_t::on_window_event(void*, const window_event_t& ev, void* user_data)
	{
		window_runtime_t& runtime = *static_cast<window_runtime_t*>(user_data);
		editor_surface_t& surface = editor_app_t::get().get_surface_by_runtime(runtime);

		if (!surface.ui)
			return;

		ui::ui_context& ui = *surface.ui;

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

		editor_settings_t& settings = editor_settings_t::get();
		if (!settings.reload())
			return false;
		_current_project = settings.get_project();

		engine_runtime_t::init_globals(64ull * 1024ull * 1024ull);

		if (!engine_runtime_t::init_backend({}))
		{
			engine_runtime_t::uninit_globals();
			return false;
		}

		resource_pack_t::init_params_t pack_params;
		pack_params.manifest_path = editor_directories_t::get_editor_manifest();
		pack_params.assets_dir	  = editor_directories_t::get_editor_assets();
		pack_params.cache_dir	  = editor_directories_t::get_editor_resource_cache();

		if (!_resource_pack.init(resource_manager_t::get(), pack_params))
		{
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		resource_manager_t::get().wait_for_all_complete();

		if (!_renderer.init())
		{
			_resource_pack.uninit();
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		const editor_layout_t& layout = settings.get_layout();
		if (layout.windows.empty())
		{
			const editor_layout_window_t window = {};
			const surface_handle_t		 handle = create_surface(window.pos, get_layout_window_size(window), editor_surface_type_e::primary);
			if (handle.is_null())
				return false;
			load_surface_default_layout(_surfaces.get(handle));
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
			load_surface_dock_layout(_surfaces.get(primary_handle), primary_window->dock_layout);

			for (const editor_layout_window_t& window : layout.windows)
			{
				if (&window == primary_window)
					continue;

				const surface_handle_t secondary_handle = create_surface(window.pos, get_layout_window_size(window), editor_surface_type_e::secondary);
				load_surface_dock_layout(_surfaces.get(secondary_handle), window.dock_layout);
			}
		}

		const surface_handle_t payload_surface = create_surface({0, 0}, {160, 24}, editor_surface_type_e::payload);
		if (payload_surface.is_null())
			return false;
		_payload_controller.init(_surfaces.get(payload_surface));
		_payload_controller.set_unhandled_listener(on_payload_unhandled, this);

		if (_surfaces.empty())
		{
			_renderer.uninit();
			_resource_pack.uninit();
			engine_runtime_t::uninit_globals();
			engine_runtime_t::uninit_backend();
			return false;
		}

		_last_tick_us = time_t::get_cpu_microseconds();

		frame_allocator_tls_t::init(MAIN_FRAME_ALLOC_SIZE);
		if (!load_project(_current_project.path.c_str()))
			get_main_surface().primary->prompt_no_project_modal();
		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		_renderer.end_render();
		resource_manager_t::get().flush();
		_payload_controller.uninit();

		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.ui)
			{
				if (surface.type == editor_surface_type_e::primary)
					surface.primary->uninit();
				if (surface.type == editor_surface_type_e::secondary)
					surface.secondary->uninit();
				surface.tooltip_controller->uninit();
				surface.modal_controller->uninit();
				surface.ui->uninit();
				surface.ui.reset();
			}

			if (!surface.swapchain.is_null())
				_renderer.destroy_swapchain(surface.swapchain);
			surface.swapchain = {};
			process::destroy_window(surface.runtime->window_handle);
		}

		_renderer.uninit();
		_resource_pack.uninit();
		_surfaces.resize_zero();
		engine_runtime_t::uninit_globals();
		engine_runtime_t::uninit_backend();
		frame_allocator_tls_t::uninit();
	}

	void editor_app_t::init_surface_ui(editor_surface_t& surface)
	{
		surface.ui = make_unique<ui::ui_context>();

		ui::ui_config_t cfg = {};
		cfg.max_widgets		= 2048;
		cfg.user_ui_scale	= 1.0f;
		cfg.dpi_scale		= surface.runtime->monitor_info.dpi_scale;

		surface.ui->init(cfg);

		ui::paint_pipelines_t pipelines	  = {};
		pipelines.default_pipeline		  = "editor/shaders/ui_default.hlsl"_hs;
		pipelines.text_pipeline			  = "editor/shaders/editor_ui_text_lcd.hlsl"_hs;
		pipelines.grayscale_text_pipeline = "editor/shaders/editor_ui_text_grayscale.hlsl"_hs;
		pipelines.sdf_pipeline			  = "editor/shaders/ui_sdf.hlsl"_hs;
		surface.ui->get_paint().set_pipelines(pipelines);
		surface.ui->set_debug_draw(_debug_mode);

		surface.tooltip_controller = make_unique<editor_tooltip_controller_t>();
		surface.tooltip_controller->init(*surface.ui);

		surface.modal_controller = make_unique<editor_modal_controller_t>();
		surface.modal_controller->init(*surface.ui);
		if (surface.type == editor_surface_type_e::primary)
		{
			surface.primary = make_unique<editor_primary_base_t>();
			surface.primary->init(*surface.ui, *surface.runtime);
			surface.primary->set_current_project_name(_current_project.name.c_str());
		}
		else if (surface.type == editor_surface_type_e::secondary)
		{
			surface.secondary = make_unique<editor_secondary_base_t>();
			surface.secondary->init(*surface.ui, *surface.runtime);
		}
	}

	void editor_app_t::load_surface_default_layout(editor_surface_t& surface)
	{
		nlohmann::json layout = {
			{"version", 1},
			{"root",
			 nlohmann::json{
				 {"type", "split"},
				 {"direction", "horizontal"},
				 {"split_value", 0.22f},
				 {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Entities"}, {"data", nlohmann::json::object()}}})}}},
				 {"positive",
				  nlohmann::json{
					  {"type", "split"},
					  {"direction", "horizontal"},
					  {"split_value", 0.72f},
					  {"negative",
					   nlohmann::json{
						   {"type", "split"},
						   {"direction", "vertical"},
						   {"split_value", 0.68f},
						   {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "World"}, {"data", nlohmann::json::object()}}})}}},
						   {"positive",
							nlohmann::json{
								{"type", "leaf"},
								{"panels",
								 nlohmann::json::array(
									 {nlohmann::json{{"type", "Assets"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Log"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Profiling"}, {"data", nlohmann::json::object()}}})}}},
					   }},
					  {"positive", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Inspector"}, {"data", nlohmann::json::object()}}})}}},
				  }},
			 }},
		};

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->get_dock_widget().from_json(layout);
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
		{
			if (surface.ui)
				surface.ui->get_paint().set_text_raster_mode(raster_mode);
		}
	}

	bool editor_app_t::is_text_subpixel_enabled() const
	{
		return editor_text_rasterization_t::is_subpixel_enabled();
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
	}

	bool editor_app_t::create_project(const char* path)
	{
		if (!editor_project_t::is_project_path(path))
			return false;

		const editor_project_t project	 = editor_project_t::make_default_project(path);
		const nlohmann::json   json_data = project;
		const string_t		   data		 = json_data.dump(4);
		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), path))
			return false;

		return load_project(path);
	}

	bool editor_app_t::load_project(const char* path)
	{
		if (!editor_project_t::is_project_path(path) || !file_system_t::exists(path))
			return false;

		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
			return false;

		editor_project_t project = {};
		doc.get_to(project);
		project.path = path;

		unload_current_project();
		_current_project					   = project;
		editor_settings_t::get().get_project() = _current_project;
		editor_settings_t::get().save();
		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::primary)
				surface.primary->set_current_project_name(_current_project.name.c_str());
		}
		return true;
	}

	bool editor_app_t::save_project()
	{
		if (!editor_project_t::is_project_path(_current_project.path.c_str()))
			return false;

		const nlohmann::json json_data = _current_project;
		const string_t		 data	   = json_data.dump(4);
		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), _current_project.path.c_str()))
			return false;

		editor_settings_t::get().get_project() = _current_project;
		editor_settings_t::get().save();
		return true;
	}

	bool editor_app_t::save_project_as(const char* path)
	{
		if (!editor_project_t::is_project_path(path))
			return false;

		const string_t old_path = _current_project.path;
		_current_project.path	= path;
		if (!save_project())
		{
			_current_project.path = old_path;
			return false;
		}
		return true;
	}

	void editor_app_t::save_layout()
	{
		editor_layout_t& layout = editor_settings_t::get().get_layout();
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
		vector_t<surface_handle_t> destroy_handles;
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

		load_surface_default_layout(get_main_surface());
		save_layout();
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

	const editor_surface_t& editor_app_t::get_main_surface() const
	{
		SFG_ASSERT(!_surfaces.empty());
		for (const editor_surface_t& surface : _surfaces)
			if (surface.type == editor_surface_type_e::primary)
				return surface;
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

	editor_modal_controller_t& editor_app_t::get_modal_controller()
	{
		return *get_main_surface().modal_controller;
	}

	const editor_modal_controller_t& editor_app_t::get_modal_controller() const
	{
		return *get_main_surface().modal_controller;
	}

	editor_payload_controller_t& editor_app_t::get_payload_controller()
	{
		return _payload_controller;
	}

	const editor_payload_controller_t& editor_app_t::get_payload_controller() const
	{
		return _payload_controller;
	}

	void editor_app_t::tick()
	{
		bool tick = true;
		while (tick)
		{
			frame_allocator_tls_t::reset();

			process::pump_os_messages();

			_resource_pack.tick();
			resource_manager_t::get().flush();

			const i64 now = time_t::get_cpu_microseconds();
			const f32 dt  = static_cast<f32>(now - _last_tick_us) / 1.0e6f;
			_last_tick_us = now;

			_payload_controller.tick();

			bool main_destroyed = false;

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime->has_flag(window_runtime_flags_e::close_requested) || main_destroyed)
				{
					_renderer.end_render();
					if (surface.type == editor_surface_type_e::primary)
						main_destroyed = true;

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

				if (!minimized && !hidden && surface.ui)
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

			_renderer.ensure_render();

			static bool b = true;

			if (b)
			{
				b = false;
				for (int a = 0; a < 50; a++)
				{
					SFG_TRACE("heall yeah tesdting uh baby lewgooo");
				}
			}
		}

		_renderer.end_render();
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size)
	{
		return create_surface(pos, size, editor_surface_type_e::secondary);
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

		init_surface_ui(surface);

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

		if (surface.ui)
		{
			if (surface.type == editor_surface_type_e::primary)
				surface.primary->uninit();
			if (surface.type == editor_surface_type_e::secondary)
				surface.secondary->uninit();
			if (surface.type == editor_surface_type_e::payload)
				_payload_controller.uninit();
			surface.tooltip_controller->uninit();
			surface.modal_controller->uninit();
			surface.ui->uninit();
			surface.ui.reset();
		}
		_renderer.destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime->window_handle);
		_surfaces.remove(handle);
	}

}
