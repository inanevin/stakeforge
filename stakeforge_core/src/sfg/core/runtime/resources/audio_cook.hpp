// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/common/type_id.hpp"

#include "core/runtime/resources/audio.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct audio_cook_config_t
	{
		audio_storage_e storage = audio_storage_e::resident;

		bool operator==(const audio_cook_config_t&) const = default;
	};

	class SFG_API audio_cooker
	{
	public:
		static bool cook_from_file(const audio_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(audio_cook_config_t);

	struct audio_cook_config_reflection_t
	{
		audio_cook_config_reflection_t();
	};

}
