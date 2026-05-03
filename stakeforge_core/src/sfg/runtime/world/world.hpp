// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class world_t
	{
	public:
		void init();
		void uninit();
		void tick(f32 delta_time);
	};
}
