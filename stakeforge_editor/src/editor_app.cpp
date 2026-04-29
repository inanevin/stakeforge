// Copyright (c) 2025 Inan Evin

#include "editor_app.hpp"

#include "editor_directories.hpp"
#include "editor_resources.hpp"
#include "editor_surface.hpp"
#include "gfx/backend/backend.hpp"
#include "io/assert.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "platform/process.hpp"
#include "platform/time.hpp"
#include "serialization/serialization.hpp"
#include "vendor/nhlohmann/json.hpp"

#include "ui/ui_context.hpp"
#include "ui/input/input_router.hpp"

#include <string>

namespace sfg
{
	namespace
	{
		constexpr const char* k_default_font_path = "../../../assets/engine/fonts/Roboto-Regular.ttf";

		editor_settings_t make_default_settings()
		{
			editor_settings_t settings = {};
			settings.windows.push_back({});
			return settings;
		}

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
		if (!surface || !surface->ui)
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
		if (!reload_settings())
			return false;

		process::init();

		if (!gfx_backend::init_instance())
		{
			process::uninit();
			return false;
		}

		if (!_resources.init())
		{
			gfx_backend::uninit_instance();
			process::uninit();
			return false;
		}

		if (!_renderer.init())
		{
			_resources.uninit();
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
			_renderer.uninit();
			_resources.uninit();
			gfx_backend::uninit_instance();
			process::uninit();
			return false;
		}

		time_t::init();
		_last_tick_us = time_t::get_cpu_microseconds();
		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		save_settings();

		_renderer.join();

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
		_resources.uninit();
		gfx_backend::uninit_instance();
		_surfaces.resize_zero();
		process::uninit();
		time_t::uninit();
	}

	void editor_app_t::init_surface_ui(editor_surface_t& surface)
	{
		surface.ui = std::make_unique<ui::ui_context>();

		ui::ui_config_t cfg = {};
		cfg.max_widgets		= 512;
		cfg.atlas_width		= 1024;
		cfg.atlas_height	= 1024;
		surface.ui->init(cfg);

		// Load a default font once and share across surfaces.
		// For simplicity we load per-surface; the editor can later switch to a shared font cache.
		ui::vg_font_config_t fcfg = {};
		fcfg.size				  = 14;
		fcfg.kind				  = ui::vg_font_kind_e::bitmap;
		_ui_font				  = surface.ui->get_fonts().load_font_from_file(k_default_font_path, fcfg);
		if (_ui_font == nullptr)
			SFG_WARN("editor: failed to load default UI font from {0}", k_default_font_path);

		{
			auto& ui   = *surface.ui;
			auto  root = ui.get_root();

			// Root layout: flow column, full size.
			auto& root_in		  = ui.get_tree().in(root);
			root_in.flow		  = ui::flow_e::column;
			root_in.child_spacing = 4.0f;
			root_in.child_margins = {8.0f, 8.0f, 8.0f, 8.0f};

			// Header panel.
			auto  header	 = ui.make_row(root);
			auto& hin		 = ui.get_tree().in(header);
			hin.size_mode_y	 = ui::axis_mode_e::fixed;
			hin.size_value.y = 28.0f;

			if (_ui_font)
			{
				ui.make_label(header, "HebeEditor", _ui_font);
				ui.make_spacer(header); // fill remaining
				ui.make_button(header, "File", _ui_font);
				ui.make_button(header, "View", _ui_font);
				ui.make_button(header, "Help", _ui_font);
			}

			// Body splitter (just a single panel for now).
			auto  body		= ui.make_panel(root);
			auto& bin		= ui.get_tree().in(body);
			bin.size_mode_x = ui::axis_mode_e::fill;
			bin.size_mode_y = ui::axis_mode_e::fill;

			if (_ui_font)
			{
				ui::vg_text_paint_t tp = {.font = _ui_font, .color = ui.get_theme().color_item_fg, .scale = 1.0f, .spacing = 0};
				ui.make_label(body, "Welcome to ui UI! Layout/input/paint/serialize hooked up.", _ui_font);
			}
		}
	}

	void editor_app_t::tick()
	{
		bool tick = true;
		while (tick)
		{
			process::pump_os_messages();

			const i64	now = time_t::get_cpu_microseconds();
			const float dt	= static_cast<f32>(now - _last_tick_us) / 1.0e6f;
			_last_tick_us	= now;

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle  = *it;
				editor_surface_t&	   surface = _surfaces.get(handle);

				if (surface.runtime.has_flag(window_runtime_flags_e::close_requested))
				{
					_renderer.join();
					destroy_surface(handle);
					continue;
				}

				if (surface.runtime.size != surface.swapchain_size)
				{
					_renderer.join();
					const bool		minimized	= surface.runtime.has_flag(window_runtime_flags_e::minimized);
					const vec2u16_t resize_size = (!minimized && surface.runtime.size.x != 0 && surface.runtime.size.y != 0) ? surface.runtime.size : vec2u16_t{1, 1};
					_renderer.resize_swapchain(surface.swapchain, resize_size, surface.runtime.monitor_info.dpi_scale);
					surface.swapchain_size = resize_size;
				}

				if (surface.ui)
				{
					const vec4f_t screen = {0.0f, 0.0f, static_cast<f32>(surface.swapchain_size.x), static_cast<f32>(surface.swapchain_size.y)};
					surface.ui->tick(screen, dt);
				}
			}

			if (_surfaces.empty())
			{
				tick = false;
				break;
			}

			vector_t<surface_render_target_t> targets;
			targets.reserve(_surfaces.size());
			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				editor_surface_t& surface = _surfaces.get(*it);
				targets.push_back({
					.swapchain = surface.swapchain,
					.canvas	   = surface.ui ? &surface.ui->get_canvas() : nullptr,
					.size	   = surface.swapchain_size,
				});
			}

			_renderer.render({targets.data(), targets.size()});
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

		if (!process::create_window("Stakeforge Editor", pos, size, window_style_e::app_window, surface.runtime))
		{
			SFG_ERR("failed creating editor surface window!");
			_surfaces.remove(handle);
			return {};
		}

		surface.swapchain	   = _renderer.create_swapchain(surface.runtime.window_handle, surface.runtime.platform_handle, surface.runtime.monitor_info.dpi_scale, surface.runtime.size);
		surface.swapchain_size = surface.runtime.size;

		init_surface_ui(surface);

		surface.runtime.event_callback			 = &editor_app_t::on_window_event;
		surface.runtime.event_callback_user_data = &surface;

		return handle;
	}

	void editor_app_t::destroy_surface(surface_handle_t handle)
	{
		editor_surface_t& surface = _surfaces.get(handle);
		if (surface.ui)
		{
			surface.ui->uninit();
			surface.ui.reset();
		}
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
