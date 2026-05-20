// Copyright (c) 2025 Inan Evin
#pragma once

#include "physical_material.hpp"

namespace sfg
{
	class ostream_t;

	class physical_material_cooker
	{
	public:
		static bool cook_from_file(const char* full_path, ostream_t& stream);
	};
}
