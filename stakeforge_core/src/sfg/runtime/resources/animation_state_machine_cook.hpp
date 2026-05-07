// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_state_machine.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

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

	void from_json(const nlohmann::json& j, animation_state_machine_cook_config_t& c);
}
