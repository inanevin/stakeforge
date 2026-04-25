// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/atomic.hpp"

namespace sfg
{
	enum class gfx_runtime_error_code : u8
	{
		none,
		backend_failed,
	};

	struct gfx_runtime_stats_t
	{
		atomic_t<u64> render_thread_id = 0;
	};

	extern gfx_runtime_stats_t g_gfx_runtime_stats;
}
