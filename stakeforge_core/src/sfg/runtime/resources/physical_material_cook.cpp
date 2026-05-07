// Copyright (c) 2025 Inan Evin

#include "physical_material_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool physical_material_cooker::cook_from_file(const physical_material_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, physical_material_cook_config_t&)
	{
	}
}
