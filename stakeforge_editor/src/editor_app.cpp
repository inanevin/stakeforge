// Copyright (c) 2025 Inan Evin

#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/ui/ui_context.hpp>
#include <sfg/ui/input/input_router.hpp>

namespace sfg
{
	namespace
	{
		ui::mouse_button_e map_button(u16 b)
		{
			if (b == 1)
				return ui::mouse_button_e::right;
			if (b == 2)
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

		engine_runtime_t::init_globals();

		if (!engine_runtime_t::init_backend())
		{
			engine_runtime_t::uninit_globals();
			return false;
		}

		resource_manager_t& resource_manager = resource_manager_t::get();
		resource_manager.init(64ull * 1024ull * 1024ull);

		resource_pack_t::init_params_t pack_params;
		pack_params.manifest_path = editor_directories_t::get_editor_manifest();
		pack_params.assets_dir	  = editor_directories_t::get_editor_assets();
		pack_params.cache_dir	  = editor_directories_t::get_editor_resource_cache();
		if (!_resource_pack.init(resource_manager, pack_params))
		{
			resource_manager.drain();
			resource_manager.uninit();
			engine_runtime_t::uninit_backend();
			engine_runtime_t::uninit_globals();
			return false;
		}

		if (!_renderer.init())
		{
			resource_manager.drain();
			_resource_pack.uninit();
			resource_manager.uninit();
			engine_runtime_t::uninit_backend();
			engine_runtime_t::uninit_globals();
			return false;
		}

		_render_targets.reserve(8);

		vector_t<monitor_info_t> monitors;
		process::get_all_monitors(monitors);
		const u16 window_count = settings.get_window_count();
		for (u16 i = 0; i < window_count; ++i)
		{
			const editor_window_settings_t& ws	  = settings.get_window(i);
			const u64						ident = ws.monitor_ident;
			auto							it	  = std::find_if(monitors.begin(), monitors.end(), [ident](const monitor_info_t& m) -> bool { return m.device_hash == ident; });

			const vec2u16_t size	 = (ws.size.x == 0 || ws.size.y == 0) ? vec2u16_t(800, 600) : ws.size;
			vec2i16_t		position = ws.position;
			if (it == monitors.end())
				position = vec2i16_t::zero;

			create_surface(position, size, i);
		}

		if (_surfaces.empty())
		{
			resource_manager.drain();
			_renderer.uninit();
			_resource_pack.uninit();
			resource_manager.uninit();
			engine_runtime_t::uninit_backend();
			engine_runtime_t::uninit_globals();
			return false;
		}

		_last_tick_us = time_t::get_cpu_microseconds();
		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		editor_settings_t::get().save();

		end_render();
		resource_manager_t::get().drain();

		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.ui)
			{
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
		resource_manager_t::get().uninit();
		_surfaces.resize_zero();
		_render_targets.resize(0);
		engine_runtime_t::uninit_backend();
		engine_runtime_t::uninit_globals();
	}

	void editor_app_t::init_surface_ui(editor_surface_t& surface)
	{
		surface.ui = make_unique<ui::ui_context>();

		ui::ui_config_t cfg = {};
		cfg.max_widgets		= 512;
		cfg.atlas_width		= 1024;
		cfg.atlas_height	= 1024;
		surface.ui->init(cfg);
	}

	void editor_app_t::tick()
	{
		frame_allocator_tls_t::init(MAIN_FRAME_ALLOC_SIZE);

		bool tick = true;
		while (tick)
		{
			frame_allocator_tls_t::reset();

			process::pump_os_messages();

			_resource_pack.tick();
			resource_manager_t::get().drain();

			const i64 now = time_t::get_cpu_microseconds();
			const f32 dt  = static_cast<f32>(now - _last_tick_us) / 1.0e6f;
			_last_tick_us = now;

			_render_targets.resize(0);

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime.has_flag(window_runtime_flags_e::close_requested))
				{
					end_render();
					destroy_surface(handle);
					continue;
				}

				const bool minimized = surface.runtime.has_flag(window_runtime_flags_e::minimized);
				if (minimized)
					continue;

				if (surface.runtime.size != surface.swapchain_size)
				{
					end_render();
					_renderer.resize_swapchain(surface.swapchain, surface.runtime.size, surface.runtime.monitor_info.dpi_scale);
					surface.swapchain_size = surface.runtime.size;
				}

				if (surface.ui)
				{
					const vec4f_t screen = {0.0f, 0.0f, static_cast<f32>(surface.swapchain_size.x), static_cast<f32>(surface.swapchain_size.y)};
					surface.ui->tick(screen, dt);
				}

				_render_targets.push_back({
					.swapchain = surface.swapchain,
					.size	   = surface.swapchain_size,
				});
			}

			if (_surfaces.empty())
			{
				tick = false;
				break;
			}

			_render_delta_time = dt;
			ensure_render_thread();
		}

		end_render();
		frame_allocator_tls_t::uninit();
	}

	void editor_app_t::start_render()
	{
		SFG_ASSERT(!_render_thread_active.load());
		_render_thread_active = true;
		_render_thread		  = std::thread(&editor_app_t::render_loop, this);
	}

	void editor_app_t::end_render()
	{
		if (!_render_thread_active.load() && !_render_thread.joinable())
			return;

		_render_thread_active = false;

		if (_render_thread.joinable())
			_render_thread.join();

		_renderer.join();
	}

	void editor_app_t::ensure_render_thread()
	{
		if (_render_thread_active.load())
			return;

		start_render();
	}

	void editor_app_t::render_loop()
	{
		frame_allocator_tls_t::init(RENDER_FRAME_ALLOC_SIZE);
		g_engine_thread_ids.render_thread_id = SFG_THIS_THREAD_ID();

		while (_render_thread_active.load())
		{
			frame_allocator_tls_t::reset();
			render_resources_t::get().drain();
			_renderer.render({_render_targets.data(), _render_targets.size()}, _render_delta_time);
			time_t::yield_thread();
		}

		g_engine_thread_ids.render_thread_id = 0;
		frame_allocator_tls_t::uninit();
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size, u16 settings_idx)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() && !SFG_IS_RENDER_RUNNING());

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

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime.window_handle, surface.runtime.platform_handle, surface.runtime.monitor_info.dpi_scale, surface.runtime.size);
		surface.swapchain_size = surface.runtime.size;
		surface.settings_idx   = settings_idx;

		init_surface_ui(surface);

		surface.runtime.event_callback			 = &editor_app_t::on_window_event;
		surface.runtime.event_callback_user_data = &surface;

		editor_window_settings_t& ws = editor_settings_t::get().get_window(settings_idx);
		ws.position					 = surface.runtime.pos;
		ws.size						 = surface.runtime.size;
		ws.monitor_ident			 = surface.runtime.monitor_info.device_hash;

		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() && !SFG_IS_RENDER_RUNNING());

		editor_surface_t& surface	  = _surfaces.get(handle);
		const u16		  removed_idx = surface.settings_idx;

		if (surface.ui)
		{
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
