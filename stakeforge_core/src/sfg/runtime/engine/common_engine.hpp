// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"

namespace sfg
{
	struct world_handle_tag
	{
	};
	using world_handle_t = pool_handle_t<u32, world_handle_tag>;
}