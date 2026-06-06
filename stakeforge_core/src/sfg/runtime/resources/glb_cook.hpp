// Copyright (c) 2025 Inan Evin
#pragma once

#include "texture_payload_type.hpp"

namespace sfg
{
	class ostream_t;

	struct glb_cook_config_t
	{
		texture_payload_type_e texture_payload_type = texture_payload_type_e::ktx2_uastc;
		bool				   combine_meshes		= false;
		bool				   generate_mipmaps		= false;
	};

	class glb_cooker
	{
	public:
		static bool cook_from_file(const glb_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

}
