// Copyright (c) 2025 Inan Evin

#include "render_globals.hpp"

namespace sfg
{
	gfx_bind_layout_handle render_globals_t::s_global_bind_layout = {};

	gfx_bind_layout_handle render_globals_t::get_global_bind_layout()
	{
		return s_global_bind_layout;
	}
}
