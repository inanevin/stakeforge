// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct audio_data_t
	{
		u32 reserved = 0;
	};

	struct audio_internals_t
	{
		u32 reserved = 0;
	};

	extern bool audio_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool audio_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void audio_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void audio_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void audio_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t audio_resource_desc;
}
