// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct animation_metadata_t
	{
		u32 reserved = 0;
	};

	struct animation_internals_t
	{
		u32 reserved = 0;
	};

	extern bool animation_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx);
	extern bool animation_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void animation_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void animation_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);
	extern void register_animation_resource(resource_manager_t& resource_manager);
}
