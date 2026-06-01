// Copyright (c) 2025 Inan Evin
#pragma once

#include "physical_material.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	class physical_material_cooker
	{
	public:
		static bool cook_from_json(const nlohmann::json& json_data, ostream_t& stream);
		static bool cook_from_file(const char* full_path, ostream_t& stream);
	};
}
