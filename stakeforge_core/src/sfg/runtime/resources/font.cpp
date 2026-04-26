// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_manager.hpp"

namespace sfg
{
	bool font_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx)
	{
		(void)entry;
		(void)data;
		(void)ctx;
		return false;
	}

	bool font_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
		return false;
	}

	void font_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void font_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		(void)entry;
		(void)ctx;
	}

	void register_font_resource(resource_manager_t& resource_manager)
	{
		resource_type_desc_t desc = {};
		desc.type				  = resource_type_t::font;
		desc.metadata_size		  = sizeof(font_metadata_t);
		desc.metadata_alignment	  = alignof(font_metadata_t);
		desc.internals_size		  = sizeof(font_internals_t);
		desc.internals_alignment  = alignof(font_internals_t);
		desc.load_cpu			  = font_load_cpu;
		desc.create_internals	  = font_create_internals;
		desc.destroy_internals	  = font_destroy_internals;
		desc.unload_cpu			  = font_unload_cpu;
		resource_manager.register_type_desc(desc);
	}
}
