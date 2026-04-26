// Copyright (c) 2025 Inan Evin

#include "particle_properties.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool particle_properties_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool particle_properties_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void particle_properties_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void particle_properties_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_particle_properties_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::particle_properties;
		desc.metadata_size		  = sizeof(particle_properties_metadata_t);
		desc.metadata_alignment	  = alignof(particle_properties_metadata_t);
		desc.internals_size		  = sizeof(particle_properties_internals_t);
		desc.internals_alignment  = alignof(particle_properties_internals_t);
		desc.load_cpu			  = particle_properties_load_cpu;
		desc.create_internals	  = particle_properties_create_internals;
		desc.destroy_internals	  = particle_properties_destroy_internals;
		desc.unload_cpu			  = particle_properties_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
