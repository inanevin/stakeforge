// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/type_id.hpp>

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

	SFG_DEFINE_TYPE_ID(audio_cook_config_t);

	struct audio_cook_config_reflection_t
	{
		audio_cook_config_reflection_t();
	};

	inline audio_cook_config_reflection_t g_reflect_audio_cook_config;
}
