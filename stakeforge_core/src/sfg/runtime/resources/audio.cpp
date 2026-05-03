// Copyright (c) 2025 Inan Evin

#include "audio.hpp"

namespace sfg
{
	bool audio_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool audio_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void audio_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void audio_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void audio_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t audio_resource_desc = {
		.type				 = resource_type_e::audio,
		.data_size			 = sizeof(audio_data_t),
		.data_alignment		 = alignof(audio_data_t),
		.internals_size		 = sizeof(audio_internals_t),
		.internals_alignment = alignof(audio_internals_t),
		.load				 = audio_load,
		.create_internals	 = audio_create_internals,
		.destroy_internals	 = audio_destroy_internals,
		.unload				 = audio_unload,
		.unload_cpu			 = audio_unload_cpu,
	};
}
