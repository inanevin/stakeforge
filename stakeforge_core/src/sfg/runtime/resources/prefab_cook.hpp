// Copyright (c) 2025 Inan Evin
#pragma once

#include "prefab.hpp"

namespace sfg
{
	class ostream_t;

	class prefab_cooker
	{
	public:
		static bool cook_from_file(const char* full_path, ostream_t& stream);
	};
}
