// Copyright (c) 2025 Inan Evin
#pragma once

#include "prefab.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	class prefab_cooker
	{
	public:
		static bool cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};
}
