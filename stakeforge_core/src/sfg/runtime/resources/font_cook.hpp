// Copyright (c) 2025 Inan Evin
#pragma once

#include "font.hpp"

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

}
