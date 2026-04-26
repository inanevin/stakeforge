// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct physical_material_metadata_t
	{
		u32 reserved = 0;
	};

	struct physical_material_internals_t
	{
		u32 reserved = 0;
	};

	extern bool physical_material_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx);
	extern bool physical_material_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void physical_material_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void physical_material_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);
	extern void register_physical_material_resource(resource_manager_t& resource_manager);
}
