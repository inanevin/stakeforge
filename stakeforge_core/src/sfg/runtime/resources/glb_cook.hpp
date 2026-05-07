// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	struct glb_cook_config_t
	{
	};

	class glb_cooker
	{
	public:
		static bool cook_from_file(const glb_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

	void from_json(const nlohmann::json& j, glb_cook_config_t& c);
}
