// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct shader_data_t
	{
		u32 reserved = 0;
	};

	struct shader_internals_t
	{
		u32 reserved = 0;
	};

	extern bool shader_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool shader_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t shader_resource_desc;
}
