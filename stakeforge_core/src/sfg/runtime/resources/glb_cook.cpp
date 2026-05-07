// Copyright (c) 2025 Inan Evin

#include "glb_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool glb_cooker::cook_from_file(const glb_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, glb_cook_config_t&)
	{
	}
}
