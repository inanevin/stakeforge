// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	class texture_sampler_cooker
	{
	public:
		static bool cook_from_json(const nlohmann::json& json_data, ostream_t& stream);
	};
}
