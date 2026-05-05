// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"

namespace sfg
{
	bool mesh_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e mesh_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void mesh_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t mesh_resource_desc = {
		.type				 = resource_type_e::mesh,
		.runtime_size		 = sizeof(mesh_runtime_t),
		.runtime_alignment	 = alignof(mesh_runtime_t),
		.internals_size		 = sizeof(mesh_internals_t),
		.internals_alignment = alignof(mesh_internals_t),
		.load				 = mesh_loader_t::load,
		.create_internals	 = mesh_loader_t::create_internals,
		.destroy_internals	 = mesh_loader_t::destroy_internals,
	};
}
