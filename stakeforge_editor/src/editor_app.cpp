// Copyright (c) 2025 Inan Evin

#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_modal_controller.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include "editor_text_rasterization.hpp"
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
		editor_surface_t* surface = static_cast<editor_surface_t*>(user_data);
		if (!surface)
			return;

		if (!surface->ui)
			return;

		ui::ui_context& ui = *surface->ui;

		switch (ev.type)
		{
		case window_event_type_e::delta:
		case window_event_type_e::mouse: {
			const vec2i16_t mp = surface->runtime.mouse_position;
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
			create_surface(window.pos, window.size, true);
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

			create_surface(primary_window->pos, get_layout_window_size(*primary_window), true);

			for (const editor_layout_window_t& window : layout.windows)
			{
				if (&window == primary_window)
					continue;

				create_surface(window.pos, get_layout_window_size(window), false);
			}
		}

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
			get_main_surface().editor.prompt_no_project_modal();
		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		save_layout();

		_renderer.end_render();
		resource_manager_t::get().flush();

		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.ui)
			{
				if (surface.has_editor_base)
					surface.editor.uninit();
				if (surface.has_dock_widget)
					surface.dock_widget.uninit();
				surface.modal_controller.uninit();
				surface.ui->uninit();
				surface.ui.reset();
			}

			if (!surface.swapchain.is_null())
				_renderer.destroy_swapchain(surface.swapchain);
			surface.swapchain = {};
			process::destroy_window(surface.runtime.window_handle);
		}

		_renderer.uninit();
		_resource_pack.uninit();
		_surfaces.resize_zero();
		engine_runtime_t::uninit_globals();
		engine_runtime_t::uninit_backend();
		frame_allocator_tls_t::uninit();
	}

	void editor_app_t::init_surface_ui(editor_surface_t& surface, bool install_editor_base)
	{
		surface.ui = make_unique<ui::ui_context>();

		ui::ui_config_t cfg = {};
		cfg.max_widgets		= 512;
		surface.ui->init(cfg);

		ui::paint_pipelines_t pipelines	  = {};
		pipelines.default_pipeline		  = "editor/shaders/ui_default.hlsl"_hs;
		pipelines.text_pipeline			  = "editor/shaders/editor_ui_text_lcd.hlsl"_hs;
		pipelines.grayscale_text_pipeline = "editor/shaders/editor_ui_text_grayscale.hlsl"_hs;
		pipelines.sdf_pipeline			  = "editor/shaders/ui_sdf.hlsl"_hs;
		surface.ui->get_paint().set_pipelines(pipelines);
		surface.ui->set_debug_draw(_debug_mode);

		surface.modal_controller.init(*surface.ui);
		if (install_editor_base)
		{
			surface.editor.init(*surface.ui);
			surface.editor.set_current_project_name(_current_project.name.c_str());
			surface.has_editor_base = true;
		}
		else
		{
			surface.dock_widget.init(*surface.ui, surface.ui->get_root());
			surface.has_dock_widget = true;
		}
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
			if (surface.has_editor_base)
				surface.editor.set_current_project_name(_current_project.name.c_str());
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
			editor_layout_window_t window = {};
			window.pos					  = surface.runtime.pos;
			window.size					  = surface.runtime.size;
			window.is_primary			  = surface.has_editor_base && !primary_saved;
			if (window.is_primary)
				primary_saved = true;
			layout.windows.push_back(window);
		}

		editor_settings_t::get().save();
	}

	editor_surface_t& editor_app_t::get_main_surface()
	{
		SFG_ASSERT(!_surfaces.empty());
		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.has_editor_base)
				return surface;
		}
		SFG_ASSERT(false);
		return *_surfaces.begin();
	}

	const editor_surface_t& editor_app_t::get_main_surface() const
	{
		SFG_ASSERT(!_surfaces.empty());
		for (const editor_surface_t& surface : _surfaces)
			if (surface.has_editor_base)
				return surface;
		SFG_ASSERT(false);
		return *_surfaces.begin();
	}

	editor_modal_controller_t& editor_app_t::get_modal_controller()
	{
		return get_main_surface().modal_controller;
	}

	const editor_modal_controller_t& editor_app_t::get_modal_controller() const
	{
		return get_main_surface().modal_controller;
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

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime.has_flag(window_runtime_flags_e::close_requested))
				{
					_renderer.end_render();
					destroy_surface(handle);
					continue;
				}

				const bool minimized = surface.runtime.has_flag(window_runtime_flags_e::minimized);

				if (minimized != surface.is_minimized)
				{
					surface.is_minimized = minimized;
					_renderer.end_render();
					_renderer.set_swapchain_minimized(surface.swapchain, minimized);
				}

				if (!minimized && surface.runtime.size != surface.swapchain_size)
				{
					_renderer.end_render();
					_renderer.resize_swapchain(surface.swapchain, surface.runtime.size, surface.runtime.monitor_info.dpi_scale);
					surface.swapchain_size = surface.runtime.size;
				}

				if (!minimized && surface.ui)
				{
					const vec4f_t screen	= {0.0f, 0.0f, static_cast<f32>(surface.swapchain_size.x), static_cast<f32>(surface.swapchain_size.y)};
					const f32	  dpi_scale = surface.runtime.monitor_info.dpi_scale > 0.0f ? surface.runtime.monitor_info.dpi_scale : 1.0f;
					if (surface.has_editor_base)
						surface.editor.update(dt);
					if (surface.has_dock_widget)
						surface.dock_widget.update(dt);
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
		}

		_renderer.end_render();
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size)
	{
		return create_surface(pos, size, false);
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size, bool install_editor_base)
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

		if (!process::create_window("Stakeforge Editor", pos, size, window_style_e::app_window, surface.runtime))
		{
			SFG_ERR("failed creating editor surface window!");
			_surfaces.remove(handle);
			return {};
		}

		init_surface_ui(surface, install_editor_base);

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime.window_handle, surface.runtime.platform_handle, surface.runtime.monitor_info.dpi_scale, surface.runtime.size, surface.ui.get());
		surface.swapchain_size = surface.runtime.size;

		surface.runtime.event_callback			 = &editor_app_t::on_window_event;
		surface.runtime.event_callback_user_data = &surface;

		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_renderer.end_render();

		editor_surface_t& surface = _surfaces.get(handle);

		if (surface.ui)
		{
			if (surface.has_editor_base)
				surface.editor.uninit();
			if (surface.has_dock_widget)
				surface.dock_widget.uninit();
			surface.modal_controller.uninit();
			surface.ui->uninit();
			surface.ui.reset();
		}
		_renderer.destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime.window_handle);
		_surfaces.remove(handle);
	}

}
