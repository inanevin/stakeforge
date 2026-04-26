// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool texture_sampler_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool texture_sampler_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void texture_sampler_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void texture_sampler_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_texture_sampler_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::texture_sampler;
		desc.metadata_size		  = sizeof(texture_sampler_metadata_t);
		desc.metadata_alignment	  = alignof(texture_sampler_metadata_t);
		desc.internals_size		  = sizeof(texture_sampler_internals_t);
		desc.internals_alignment  = alignof(texture_sampler_internals_t);
		desc.load_cpu			  = texture_sampler_load_cpu;
		desc.create_internals	  = texture_sampler_create_internals;
		desc.destroy_internals	  = texture_sampler_destroy_internals;
		desc.unload_cpu			  = texture_sampler_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
