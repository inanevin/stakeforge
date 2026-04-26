// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct prefab_metadata_t
	{
		u32 reserved = 0;
	};

	struct prefab_internals_t
	{
		u32 reserved = 0;
	};

	extern bool prefab_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx);
	extern bool prefab_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void prefab_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void prefab_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);
	extern void register_prefab_resource(resource_manager_t& resource_manager);
}
