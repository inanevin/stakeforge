// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class world_t
	{
	public:
		void init();
		void uninit();
		void tick(f32 delta_time);
	};

	void to_json(nlohmann::json& j, const world_t& world);
	void from_json(const nlohmann::json& j, world_t& world);
}
