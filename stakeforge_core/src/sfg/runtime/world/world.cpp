// Copyright (c) 2025 Inan Evin

#include "world.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void world_t::init()
	{
	}

	void world_t::uninit()
	{
	}

	void world_t::tick(f32 delta_time)
	{
		(void)delta_time;
	}

	void to_json(nlohmann::json& j, const world_t&)
	{
		j = nlohmann::json::object();
	}

	void from_json(const nlohmann::json&, world_t&)
	{
	}
}
