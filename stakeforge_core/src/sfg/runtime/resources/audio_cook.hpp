// Copyright (c) 2025 Inan Evin
#pragma once

#include "audio.hpp"

namespace sfg
{
	class ostream_t;

	struct audio_cook_config_t
	{
	};

	class audio_cooker
	{
	public:
		static bool cook_from_file(const audio_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

}
