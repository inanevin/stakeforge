// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	class engine_runtime_t;

	class render_globals_t
	{
	public:
		static gfx_handle_t get_global_bind_layout();

	private:
		friend class engine_runtime_t;

		static gfx_handle_t s_global_bind_layout;
	};
}
