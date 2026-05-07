// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool material_cooker::cook_from_file(const material_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, material_cook_config_t&)
	{
	}
}
