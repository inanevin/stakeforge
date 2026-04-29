// Copyright (c) 2025 Inan Evin

#include "audio.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool audio_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool audio_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void audio_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void audio_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_audio_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_e::audio;
		desc.metadata_size		  = sizeof(audio_metadata_t);
		desc.metadata_alignment	  = alignof(audio_metadata_t);
		desc.internals_size		  = sizeof(audio_internals_t);
		desc.internals_alignment  = alignof(audio_internals_t);
		desc.load_cpu			  = audio_load_cpu;
		desc.create_internals	  = audio_create_internals;
		desc.destroy_internals	  = audio_destroy_internals;
		desc.unload_cpu			  = audio_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
