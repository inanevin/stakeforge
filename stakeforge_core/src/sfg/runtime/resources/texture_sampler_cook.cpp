// Copyright (c) 2025 Inan Evin

#include "texture_sampler_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool texture_sampler_cooker::cook_from_file(const texture_sampler_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, texture_sampler_cook_config_t&)
	{
	}
}
