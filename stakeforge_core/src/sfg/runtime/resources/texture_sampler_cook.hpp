// Copyright (c) 2025 Inan Evin
#pragma once

#include "texture_sampler.hpp"

namespace sfg
{
	class ostream_t;

	class texture_sampler_cooker
	{
	public:
		static bool cook_from_file(const char* full_path, ostream_t& stream);
	};
}
