// Copyright (c) 2025 Inan Evin

#include "audio.hpp"

namespace sfg
{
	bool audio_loader_t::load(resource_entry_t&, resource_context_t&, ostream_t&)
	{
		return false;
	}

	bool audio_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e audio_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void audio_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t audio_resource_desc = {
		.type				 = resource_type_e::audio,
		.runtime_size		 = sizeof(audio_runtime_t),
		.runtime_alignment	 = alignof(audio_runtime_t),
		.internals_size		 = sizeof(audio_internals_t),
		.internals_alignment = alignof(audio_internals_t),
		.initial_load_offset = 0,
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.async_load			 = false,
		.load				 = audio_loader_t::load,
		.load_v2			 = audio_loader_t::load,
		.create_internals	 = audio_loader_t::create_internals,
		.destroy_internals	 = audio_loader_t::destroy_internals,
	};
}
