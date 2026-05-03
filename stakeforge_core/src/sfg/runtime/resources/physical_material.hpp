// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct physical_material_data_t
	{
		u32 reserved = 0;
	};

	struct physical_material_internals_t
	{
		u32 reserved = 0;
	};

	extern bool physical_material_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool physical_material_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void physical_material_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void physical_material_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void physical_material_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t physical_material_resource_desc;
}
