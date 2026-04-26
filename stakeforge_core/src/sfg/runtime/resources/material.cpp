// Copyright (c) 2025 Inan Evin

#include "material.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool material_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool material_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void material_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void material_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_material_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::material;
		desc.metadata_size		  = sizeof(material_metadata_t);
		desc.metadata_alignment	  = alignof(material_metadata_t);
		desc.internals_size		  = sizeof(material_internals_t);
		desc.internals_alignment  = alignof(material_internals_t);
		desc.load_cpu			  = material_load_cpu;
		desc.create_internals	  = material_create_internals;
		desc.destroy_internals	  = material_destroy_internals;
		desc.unload_cpu			  = material_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
