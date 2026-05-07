// Copyright (c) 2025 Inan Evin

#include "prefab_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool prefab_cooker::cook_from_file(const prefab_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, prefab_cook_config_t&)
	{
	}
}
