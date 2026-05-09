// Copyright (c) 2025 Inan Evin
#pragma once

#include "font.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	struct font_cook_config_t
	{
		u32 reserved = 0;
	};

	class font_cooker
	{
	public:
		static bool cook_from_file(const font_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

	void from_json(const nlohmann::json& j, font_cook_config_t& c);
}
