// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_editor.hpp"
#include "editor_renderer.hpp"
#include "editor_settings.hpp"
#include "editor_surface.hpp"
#include "memory/dynamic_pool_allocator_gen.hpp"

namespace sfg
{
	class editor_app_t
	{
	public:
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

	private:
		using surface_pool_t = dynamic_pool_gen_t<editor_surface_t, u16, editor_surface_tag_t>;

		editor_renderer_t _renderer;
		editor_settings_t _settings;
		surface_pool_t	  _surfaces;
	};
}
