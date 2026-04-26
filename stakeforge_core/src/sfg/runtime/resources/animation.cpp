// Copyright (c) 2025 Inan Evin

#include "animation.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool animation_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool animation_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void animation_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void animation_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_animation_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::animation;
		desc.metadata_size		  = sizeof(animation_metadata_t);
		desc.metadata_alignment	  = alignof(animation_metadata_t);
		desc.internals_size		  = sizeof(animation_internals_t);
		desc.internals_alignment  = alignof(animation_internals_t);
		desc.load_cpu			  = animation_load_cpu;
		desc.create_internals	  = animation_create_internals;
		desc.destroy_internals	  = animation_destroy_internals;
		desc.unload_cpu			  = animation_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
