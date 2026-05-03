// Copyright (c) 2025 Inan Evin
#pragma once

#include "texture.hpp"
#include <sfg/data/vector.hpp>

namespace sfg
{
	class ostream_t;

	struct texture_cook_mip_t
	{
		vector_t<u8> pixels;
		u32			 width	= 0;
		u32			 height = 0;
	};

	struct texture_cook_t
	{
		texture_cook_mip_t mips[16]	 = {};
		u32				   width	 = 0;
		u32				   height	 = 0;
		u8				   channels	 = 0;
		u8				   mip_count = 0;
		bool			   is_linear = false;
	};

	bool texture_cook_from_file(const char* full_path, const texture_config_t& cfg, texture_cook_t& out);
	bool texture_cook_serialize(const texture_cook_t& src, ostream_t& stream);
}
