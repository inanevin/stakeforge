// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct prefab_data_t
	{
		u32 reserved = 0;
	};

	struct prefab_internals_t
	{
		u32 reserved = 0;
	};

	extern bool prefab_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool prefab_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void prefab_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void prefab_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void prefab_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t prefab_resource_desc;
}
