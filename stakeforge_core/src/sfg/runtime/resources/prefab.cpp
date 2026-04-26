// Copyright (c) 2025 Inan Evin

#include "prefab.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool prefab_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool prefab_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void prefab_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void prefab_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_prefab_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::prefab;
		desc.metadata_size		  = sizeof(prefab_metadata_t);
		desc.metadata_alignment	  = alignof(prefab_metadata_t);
		desc.internals_size		  = sizeof(prefab_internals_t);
		desc.internals_alignment  = alignof(prefab_internals_t);
		desc.load_cpu			  = prefab_load_cpu;
		desc.create_internals	  = prefab_create_internals;
		desc.destroy_internals	  = prefab_destroy_internals;
		desc.unload_cpu			  = prefab_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
