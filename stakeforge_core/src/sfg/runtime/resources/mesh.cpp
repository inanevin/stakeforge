// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool mesh_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool mesh_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void mesh_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void mesh_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_mesh_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_e::mesh;
		desc.metadata_size		  = sizeof(mesh_metadata_t);
		desc.metadata_alignment	  = alignof(mesh_metadata_t);
		desc.internals_size		  = sizeof(mesh_internals_t);
		desc.internals_alignment  = alignof(mesh_internals_t);
		desc.load_cpu			  = mesh_load_cpu;
		desc.create_internals	  = mesh_create_internals;
		desc.destroy_internals	  = mesh_destroy_internals;
		desc.unload_cpu			  = mesh_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
