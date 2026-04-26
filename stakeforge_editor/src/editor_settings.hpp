// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/string.hpp"
#include "data/vector.hpp"
#include "math/vec2i16.hpp"
#include "math/vec2u16.hpp"

#include "vendor/nhlohmann/json_fwd.hpp"

namespace sfg
{
	struct editor_window_settings_t
	{
		vec2i16_t position		= {64, 64};
		vec2u16_t size			= {1280, 720};
		u64		  monitor_ident = UINT64_MAX;
	};

	struct editor_settings_t
	{
		vector_t<editor_window_settings_t> windows;
	};

	void to_json(nlohmann::json& j, const editor_window_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_window_settings_t& settings);
	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
