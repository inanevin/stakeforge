// Copyright (c) 2025 Inan Evin
#pragma once

#include "material.hpp"

namespace sfg
{
	class ostream_t;

	struct material_cook_config_t
	{
	};

	class material_cooker
	{
	public:
		static bool cook_from_file(const material_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};
}
