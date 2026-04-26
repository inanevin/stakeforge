// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool texture_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool texture_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void texture_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void texture_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_texture_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::texture;
		desc.metadata_size		  = sizeof(texture_metadata_t);
		desc.metadata_alignment	  = alignof(texture_metadata_t);
		desc.internals_size		  = sizeof(texture_internals_t);
		desc.internals_alignment  = alignof(texture_internals_t);
		desc.load_cpu			  = texture_load_cpu;
		desc.create_internals	  = texture_create_internals;
		desc.destroy_internals	  = texture_destroy_internals;
		desc.unload_cpu			  = texture_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
