// Copyright (c) 2025 Inan Evin
#pragma once

#include "particle_properties.hpp"

namespace sfg
{
	class ostream_t;

	class particle_properties_cooker
	{
	public:
		static bool cook_from_file(const char* full_path, ostream_t& stream);
	};
}
