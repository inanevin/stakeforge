// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_editor.hpp"
#include "editor_renderer.hpp"
#include "editor_resources.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include "memory/dynamic_pool_allocator_gen.hpp"
#include "ui/vg/vg_font.hpp"

namespace sfg
{
	class editor_app_t
	{
	public:
		editor_app_t()								 = default;
		~editor_app_t()								 = default;
		editor_app_t(const editor_app_t&)			 = delete;
		editor_app_t& operator=(const editor_app_t&) = delete;

		inline static editor_app_t& get()
		{
			static editor_app_t s_instance;
			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// surface
		// -----------------------------------------------------------------------------

		surface_handle_t create_surface(const vec2i16_t& pos, const vec2u16_t& size);
		void			 destroy_surface(surface_handle_t handle);

		// -----------------------------------------------------------------------------
		// settings
		// -----------------------------------------------------------------------------

		bool reload_settings();
		void save_settings();
		void flush_settings_to_disk();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_resources_t& get_resources()
		{
			return _resources;
		}
		inline const editor_resources_t& get_resources() const
		{
			return _resources;
		}

	private:
		void		init_surface_ui(editor_surface_t& surface);
		static void on_window_event(void* hwnd, const struct window_event_t& ev, void* user_data);

	private:
		using surface_pool_t = dynamic_pool_gen_t<editor_surface_t, u16, editor_surface_tag_t>;

		editor_renderer_t  _renderer;
		editor_resources_t _resources;
		surface_pool_t	   _surfaces;
		editor_settings_t  _settings;

		ui::vg_font_t* _ui_font	   = nullptr;
		i64				 _last_tick_us = 0;
	};
}
