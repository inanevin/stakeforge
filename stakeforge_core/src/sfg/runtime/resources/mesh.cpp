// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"

namespace sfg
{
	bool mesh_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool mesh_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void mesh_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void mesh_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void mesh_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t mesh_resource_desc = {
		.type				 = resource_type_e::mesh,
		.data_size			 = sizeof(mesh_data_t),
		.data_alignment		 = alignof(mesh_data_t),
		.internals_size		 = sizeof(mesh_internals_t),
		.internals_alignment = alignof(mesh_internals_t),
		.load				 = mesh_load,
		.create_internals	 = mesh_create_internals,
		.destroy_internals	 = mesh_destroy_internals,
		.unload				 = mesh_unload,
		.unload_cpu			 = mesh_unload_cpu,
	};
}
