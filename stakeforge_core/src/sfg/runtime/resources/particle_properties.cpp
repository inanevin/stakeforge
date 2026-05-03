// Copyright (c) 2025 Inan Evin

#include "particle_properties.hpp"

namespace sfg
{
	bool particle_properties_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool particle_properties_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void particle_properties_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void particle_properties_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void particle_properties_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t particle_properties_resource_desc = {
		.type				 = resource_type_e::particle_properties,
		.data_size			 = sizeof(particle_properties_data_t),
		.data_alignment		 = alignof(particle_properties_data_t),
		.internals_size		 = sizeof(particle_properties_internals_t),
		.internals_alignment = alignof(particle_properties_internals_t),
		.load				 = particle_properties_load,
		.create_internals	 = particle_properties_create_internals,
		.destroy_internals	 = particle_properties_destroy_internals,
		.unload				 = particle_properties_unload,
		.unload_cpu			 = particle_properties_unload_cpu,
	};
}
