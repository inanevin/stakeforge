// Copyright (c) 2025 Inan Evin

#include "editor_app.hpp"

#include "editor_directories.hpp"
#include "gfx/backend/backend.hpp"
#include "io/assert.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "platform/process.hpp"
#include "serialization/serialization.hpp"
#include "vendor/nhlohmann/json.hpp"

#include <cstdio>
#include <exception>
#include <string>

namespace sfg
{
	namespace
	{
		editor_settings_t make_default_settings()
		{
			editor_settings_t settings = {};
			settings.windows.push_back({});
			return settings;
		}
	}

	bool editor_app_t::init()
	{
		if (!reload_settings())
			return false;

		process::init();

		if (!gfx_backend::init_instance())
		{
			process::uninit();
			return false;
		}

		if (!_renderer.init())
		{
			gfx_backend::uninit_instance();
			process::uninit();
			return false;
		}

		// create windows saved from settings.
		vector_t<monitor_info_t> monitors;
		process::get_all_monitors(monitors);
		for (const editor_window_settings_t& ws : _settings.windows)
		{
			const u64 ident = ws.monitor_ident;
			auto	  it	= std::find_if(monitors.begin(), monitors.end(), [ident](const monitor_info_t& m) -> bool { return m.device_hash == ident; });

			const vec2u16_t size	 = (ws.size.x == 0 || ws.size.y == 0) ? vec2u16_t(800, 600) : ws.size;
			vec2i16_t		position = ws.position;
			if (it == monitors.end())
				position = vec2i16_t::zero;

			create_surface(position, size);
		}
		if (_surfaces.empty())
		{
			gfx_backend::uninit_instance();
			process::uninit();
			return false;
		}

		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		save_settings();

		for (editor_surface_t& surface : _surfaces)
		{
			_renderer.destroy_swapchain(surface.swapchain);
			surface.swapchain = {};
			process::destroy_window(surface.runtime.window_handle);
		}

		_renderer.uninit();
		gfx_backend::uninit_instance();
		_surfaces.resize_zero();
		process::uninit();
	}

	void editor_app_t::tick()
	{
		bool tick = true;
		while (tick)
		{
			process::pump_os_messages();

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime.has_flag(window_runtime_flags_t::close_requested))
				{
					_renderer.join();
					destroy_surface(handle);
					continue;
				}

				if (surface.runtime.size != surface.swapchain_size)
				{
					_renderer.join();
					const bool		minimized	= surface.runtime.has_flag(window_runtime_flags_t::minimized);
					const vec2u16_t resize_size = (!minimized && surface.runtime.size.x != 0 && surface.runtime.size.y != 0) ? surface.runtime.size : vec2u16_t{1, 1};
					_renderer.resize_swapchain(surface.swapchain, resize_size, surface.runtime.monitor_info.dpi_scale);
					surface.swapchain_size = resize_size;
				}
			}

			if (_surfaces.empty())
			{
				tick = false;
				break;
			}

			_renderer.render();
		}
	}

	surface_handle_t editor_app_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size)
	{
		vector_t<monitor_info_t> monitors;
		process::get_all_monitors(monitors);

		if (size.x == 0 || size.y == 0)
		{
			SFG_ERR("can't create a surface with zero size.");
			return {};
		}

		const surface_handle_t handle  = _surfaces.add();
		editor_surface_t&	   surface = _surfaces.get(handle);
		surface						   = {};

		if (!process::create_window("Stakeforge Editor", pos, size, window_style_t::app_window, surface.runtime))
		{
			SFG_ERR("failed creating editor surface window!");
			_surfaces.remove(handle);
			return {};
		}

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime.window_handle, surface.runtime.platform_handle, surface.runtime.monitor_info.dpi_scale, surface.runtime.size);
		surface.swapchain_size = surface.runtime.size;
		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		editor_surface_t& surface = _surfaces.get(handle);
		_renderer.destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime.window_handle);
		_surfaces.remove(handle);
	}

	bool editor_app_t::reload_settings()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system::exists(directory.c_str()) && !file_system::create_directory(directory.c_str()))
			return false;

		const string_t path = editor_directories_t::get_settings_path();
		if (!file_system::exists(path.c_str()))
		{
			_settings = make_default_settings();
			flush_settings_to_disk();
			return true;
		}

		try
		{
			const string_t data = file_system::read_file_as_string(path.c_str());
			_settings			= nlohmann::json::parse(data).get<editor_settings_t>();
		}
		catch (const std::exception& e)
		{
			SFG_ERR("failed loading editor settings: {0}", e.what());
			_settings = make_default_settings();
			flush_settings_to_disk();
			return true;
		}

		if (_settings.windows.empty())
			_settings.windows.push_back({});

		return true;
	}

	void editor_app_t::save_settings()
	{
		_settings.windows.resize(0);

		for (editor_surface_t& surface : _surfaces)
		{
			_settings.windows.push_back({
				.position	   = surface.runtime.pos,
				.size		   = surface.runtime.size,
				.monitor_ident = surface.runtime.monitor_info.device_hash,
			});
		}

		flush_settings_to_disk();
	}

	void editor_app_t::flush_settings_to_disk()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system::exists(directory.c_str()) && !file_system::create_directory(directory.c_str()))
			return;

		const nlohmann::json json_data = _settings;
		const string_t		 data	   = json_data.dump(4);
		serialization::write_to_file(string_view_t(data.data(), data.size()), editor_directories_t::get_settings_path().c_str());
	}
}
