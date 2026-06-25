// Copyright (c) 2025 Inan Evin

#include "audio.hpp"

namespace sfg
{
	bool audio_loader_t::load(resource_entry_t&, resource_context_t&, resource_file_system_t&)
	{
		return false;
	}

	void audio_loader_t::unload(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t audio_resource_desc = {
		.type				 = resource_type_e::audio,
		.runtime_size		 = sizeof(audio_runtime_t),
		.runtime_alignment	 = alignof(audio_runtime_t),
		.internals_size		 = sizeof(audio_internals_t),
		.internals_alignment = alignof(audio_internals_t),
		.use_async_load		 = false,
		.load				 = audio_loader_t::load,
		.unload				 = audio_loader_t::unload,
	};
}
