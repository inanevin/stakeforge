// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_state_machine.hpp"

namespace sfg
{
	class ostream_t;

	struct animation_state_machine_cook_config_t
	{
	};

	class animation_state_machine_cooker
	{
	public:
		static bool cook_from_file(const animation_state_machine_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};
}
