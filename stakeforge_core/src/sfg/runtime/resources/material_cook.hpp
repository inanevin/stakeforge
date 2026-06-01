// Copyright (c) 2025 Inan Evin
#pragma once

#include "material.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	class material_cooker
	{
	public:
		static bool cook_from_json(const nlohmann::json& json_data, ostream_t& stream);
	};
}
