// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/type_id.hpp>

#include "font.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct font_cook_config_t
	{
		u32 reserved = 0;
	};

	class font_cooker
	{
	public:
		static bool cook_from_file(const font_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(font_cook_config_t);

	struct font_cook_config_reflection_t
	{
		font_cook_config_reflection_t();
	};

	inline font_cook_config_reflection_t g_reflect_font_cook_config;
}
