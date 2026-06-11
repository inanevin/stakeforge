// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/type_id.hpp>

#include "texture_payload_type.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct glb_cook_config_t
	{
		texture_payload_type_e texture_payload_type = texture_payload_type_e::ktx2_uastc;
		bool				   combine_meshes		= false;
		bool				   generate_mipmaps		= false;
	};

	class glb_cooker
	{
	public:
		static bool cook_from_file(const glb_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(glb_cook_config_t);

	struct glb_cook_config_reflection_t
	{
		glb_cook_config_reflection_t();
	};

	inline glb_cook_config_reflection_t g_reflect_glb_cook_config;
}
