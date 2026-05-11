// Copyright (c) 2025 Inan Evin

#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_modal_controller.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
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

		switch (ev.type)
		{
		case window_event_type_e::resize:
			editor_settings_t::get().get_window(surface->settings_idx).size = surface->runtime.size;
			return;
		case window_event_type_e::repos:
			editor_settings_t::get().get_window(surface->settings_idx).position = surface->runtime.pos;
			return;
		case window_event_type_e::display_change:
			editor_settings_t::get().get_window(surface->settings_idx).monitor_ident = surface->runtime.monitor_info.device_hash;
			return;
		default:
			break;
		}

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

		vector_t<monitor_info_t> monitors;
		process::get_all_monitors(monitors);
		const u16 window_count = settings.get_window_count();
		for (u16 i = 0; i < window_count; ++i)
		{
			const editor_window_settings_t& ws	  = settings.get_window(i);
			const u64						ident = ws.monitor_ident;
			auto							it	  = std::find_if(monitors.begin(), monitors.end(), [ident](const monitor_info_t& m) -> bool { return m.device_hash == ident; });

			const vec2u16_t size	 = (ws.size.x == 0 || ws.size.y == 0) ? vec2u16_t(1920, 1080) : ws.size;
			vec2i16_t		position = ws.position;
			if (it == monitors.end())
				position = vec2i16_t::zero;

			create_surface(position, size, i);
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
			get_primary_surface().editor.prompt_no_project_modal();
		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		editor_settings_t::get().save();

		_renderer.end_render();
		resource_manager_t::get().flush();

		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.ui)
			{
				surface.editor.uninit();
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

	void editor_app_t::init_surface_ui(editor_surface_t& surface)
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
		surface.editor.init(*surface.ui, surface);
		surface.editor.set_current_project_name(_current_project.name.c_str());
	}

	void editor_app_t::set_debug_mode(bool enabled)
	{
		_debug_mode = enabled;
		for (editor_surface_t& surface : _surfaces)
			surface.ui->set_debug_draw(enabled);
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
			surface.editor.set_current_project_name(_current_project.name.c_str());
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

	editor_surface_t& editor_app_t::get_primary_surface()
	{
		SFG_ASSERT(!_surfaces.empty());
		return *_surfaces.begin();
	}

	editor_modal_controller_t& editor_app_t::get_modal_controller(editor_surface_t& surface)
	{
		for (editor_surface_t& candidate : _surfaces)
		{
			if (&candidate == &surface)
				return candidate.modal_controller;
		}
		SFG_ASSERT(false);
		return surface.modal_controller;
	}

	const editor_modal_controller_t& editor_app_t::get_modal_controller(const editor_surface_t& surface) const
	{
		for (const editor_surface_t& candidate : _surfaces)
		{
			if (&candidate == &surface)
				return candidate.modal_controller;
		}
		SFG_ASSERT(false);
		return surface.modal_controller;
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
		return create_surface(pos, size, UINT16_MAX);
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size, u16 settings_idx)
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

		init_surface_ui(surface);

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime.window_handle, surface.runtime.platform_handle, surface.runtime.monitor_info.dpi_scale, surface.runtime.size, surface.ui.get());
		surface.swapchain_size = surface.runtime.size;
		surface.settings_idx   = settings_idx == UINT16_MAX ? editor_settings_t::get().add_window({
																  .position		 = surface.runtime.pos,
																  .size			 = surface.runtime.size,
																  .monitor_ident = surface.runtime.monitor_info.device_hash,
															  })
															: settings_idx;

		surface.runtime.event_callback			 = &editor_app_t::on_window_event;
		surface.runtime.event_callback_user_data = &surface;

		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_renderer.end_render();

		editor_surface_t& surface	  = _surfaces.get(handle);
		const u16		  removed_idx = surface.settings_idx;

		if (surface.ui)
		{
			surface.editor.uninit();
			surface.modal_controller.uninit();
			surface.ui->uninit();
			surface.ui.reset();
		}
		_renderer.destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime.window_handle);
		_surfaces.remove(handle);

		editor_settings_t::get().remove_window(removed_idx);

		for (editor_surface_t& other : _surfaces)
		{
			if (other.settings_idx > removed_idx)
				other.settings_idx--;
		}
	}

}
