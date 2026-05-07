// Copyright (c) 2025 Inan Evin
#pragma once

#include "texture.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	struct texture_cook_config_t
	{
		bool generate_mipmaps = false;
		bool is_linear		  = false;
	};

	class texture_cooker
	{
	public:
		static bool cook_from_file(const texture_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

	void from_json(const nlohmann::json& j, texture_cook_config_t& c);
}
