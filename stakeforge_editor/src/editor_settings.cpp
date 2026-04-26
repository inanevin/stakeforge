// Copyright (c) 2025 Inan Evin

#include "editor_settings.hpp"

#include "common/size_definitions.hpp"
#include "vendor/nhlohmann/json.hpp"

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_window_settings_t& settings)
	{
		j["position"]			= nlohmann::json::array_t({settings.position.x, settings.position.y});
		j["size"]				= settings.size;
		j["monitor_identifier"] = settings.monitor_ident;
	}

	void from_json(const nlohmann::json& j, editor_window_settings_t& settings)
	{
		const nlohmann::json position = j.value("position", nlohmann::json::array_t({64, 64}));
		if (position.is_array() && position.size() >= 2)
			settings.position = {position.at(0).get<i16>(), position.at(1).get<i16>()};

		settings.size		   = j.value("size", vec2u16_t{1280, 720});
		settings.monitor_ident = j.value<u64>("monitor_identifier", UINT64_MAX);
	}

	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["windows"] = settings.windows;
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings.windows = j.value("windows", vector_t<editor_window_settings_t>{});
	}
}
