// Copyright (c) 2025 Inan Evin
#pragma once

#include "prefab.hpp"

namespace sfg
{
	class ostream_t;

	struct prefab_cook_config_t
	{
	};

	class prefab_cooker
	{
	public:
		static bool cook_from_file(const prefab_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};
}
