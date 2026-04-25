// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "stakeforge_api_common.h"

namespace sfg
{
	enum class engine_runtime_error_code : u8
	{
		none,
		renderer_already_init,
		backend_failed,
	};

	inline constexpr engine_config_t default_engine_config()
	{
		return {
			.fixed_framerate_ns		   = 16'666'667.0,
			.fixed_framerate_max_ticks = 4,
			.frame_allocator_size	   = 1024ull * 1024ull * 4ull,
		};
	}

	extern engine_config_t g_engine_runtime_config;
}
