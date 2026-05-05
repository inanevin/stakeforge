// Copyright (c) 2025 Inan Evin

#include "particle_properties.hpp"

namespace sfg
{
	bool particle_properties_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e particle_properties_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void particle_properties_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t particle_properties_resource_desc = {
		.type				 = resource_type_e::particle_properties,
		.runtime_size		 = sizeof(particle_properties_runtime_t),
		.runtime_alignment	 = alignof(particle_properties_runtime_t),
		.internals_size		 = sizeof(particle_properties_internals_t),
		.internals_alignment = alignof(particle_properties_internals_t),
		.load				 = particle_properties_loader_t::load,
		.create_internals	 = particle_properties_loader_t::create_internals,
		.destroy_internals	 = particle_properties_loader_t::destroy_internals,
	};
}
