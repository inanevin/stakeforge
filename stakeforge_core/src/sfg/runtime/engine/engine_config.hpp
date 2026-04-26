// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"

namespace sfg
{
	enum class engine_runtime_error_code : u8
	{
		none,
		renderer_already_init,
		backend_failed,
	};

	struct engine_config_t
	{
		double fixed_framerate_ns		 = 16'666'667.0;
		size_t frame_allocator_size		 = 1024ull * 1024ull * 4ull;
		size_t resource_allocator_size	 = 1024ull * 1024ull * 64ull;
		u8	   fixed_framerate_max_ticks = 4;
	};
}
