// Copyright (c) 2025 Inan Evin
#pragma once

namespace sfg
{
	class ostream_t;

	struct glb_cook_config_t
	{
	};

	class glb_cooker
	{
	public:
		static bool cook_from_file(const glb_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

}
