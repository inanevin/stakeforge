// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct particle_properties_metadata_t
	{
		u32 reserved = 0;
	};

	struct particle_properties_internals_t
	{
		u32 reserved = 0;
	};

	extern bool particle_properties_load_cpu(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx);
	extern bool particle_properties_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void particle_properties_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void particle_properties_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);
	extern void register_particle_properties_resource(resource_manager_t& resource_manager);
}
