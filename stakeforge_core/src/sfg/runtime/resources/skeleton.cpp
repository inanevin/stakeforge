// Copyright (c) 2025 Inan Evin

#include "skeleton.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool skeleton_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool skeleton_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void skeleton_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void skeleton_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_skeleton_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_e::skeleton;
		desc.metadata_size		  = sizeof(skeleton_metadata_t);
		desc.metadata_alignment	  = alignof(skeleton_metadata_t);
		desc.internals_size		  = sizeof(skeleton_internals_t);
		desc.internals_alignment  = alignof(skeleton_internals_t);
		desc.load_cpu			  = skeleton_load_cpu;
		desc.create_internals	  = skeleton_create_internals;
		desc.destroy_internals	  = skeleton_destroy_internals;
		desc.unload_cpu			  = skeleton_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
