// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct mesh_data_t
	{
		u32 reserved = 0;
	};

	struct mesh_internals_t
	{
		u32 reserved = 0;
	};

	extern bool mesh_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool mesh_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void mesh_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void mesh_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void mesh_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t mesh_resource_desc;
}
