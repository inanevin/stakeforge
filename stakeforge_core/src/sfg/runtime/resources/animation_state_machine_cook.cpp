// Copyright (c) 2025 Inan Evin

#include "animation_state_machine_cook.hpp"

#include <sfg/io/log.hpp>

namespace sfg
{
	bool animation_state_machine_cooker::cook_from_file(const char*, resource_header_t&, ostream_t&)
	{
		SFG_ERR("animation state machine cooking is not implemented");
		return false;
	}
}
