// Copyright (c) 2025 Inan Evin
#pragma once

#include "texture.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	enum class texture_cook_payload_type_e : u8
	{
		uncompressed,
		ktx2_uastc,
	};

	struct texture_cook_config_t
	{
		texture_cook_payload_type_e payload_type	 = {};
		bool						generate_mipmaps = false;
		bool						is_linear		 = false;
	};

	class texture_cooker
	{
	public:
		static bool cook_from_file(const texture_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

	void to_json(nlohmann::json& j, const texture_cook_payload_type_e& e);
	void from_json(const nlohmann::json& j, texture_cook_payload_type_e& e);
	void to_json(nlohmann::json& j, const texture_cook_config_t& c);
	void from_json(const nlohmann::json& j, texture_cook_config_t& c);
}
