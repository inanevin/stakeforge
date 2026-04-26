// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool shader_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool shader_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void shader_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void shader_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_shader_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::shader;
		desc.metadata_size		  = sizeof(shader_metadata_t);
		desc.metadata_alignment	  = alignof(shader_metadata_t);
		desc.internals_size		  = sizeof(shader_internals_t);
		desc.internals_alignment  = alignof(shader_internals_t);
		desc.load_cpu			  = shader_load_cpu;
		desc.create_internals	  = shader_create_internals;
		desc.destroy_internals	  = shader_destroy_internals;
		desc.unload_cpu			  = shader_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
