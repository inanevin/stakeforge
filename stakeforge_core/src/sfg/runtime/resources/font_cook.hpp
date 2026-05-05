// Copyright (c) 2025 Inan Evin
#pragma once

#include "font.hpp"

namespace sfg
{
	class ostream_t;

	struct font_cook_config_t
	{
		u32			size		 = 16;
		u32			range_start	 = 32;
		u32			range_end	 = 128;
		i32			sdf_padding	 = 3;
		i32			sdf_edge	 = 128;
		f32			sdf_distance = 32.0f;
		font_kind_e kind		 = font_kind_e::bitmap;
	};

	class font_cooker
	{
	public:
		static bool cook_from_file(const font_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};
}
