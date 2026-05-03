// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct material_data_t
	{
		u32 reserved = 0;
	};

	struct material_internals_t
	{
		u32 reserved = 0;
	};

	extern bool material_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool material_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void material_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void material_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void material_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t material_resource_desc;
}
