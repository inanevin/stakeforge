// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct particle_properties_data_t
	{
		u32 reserved = 0;
	};

	struct particle_properties_internals_t
	{
		u32 reserved = 0;
	};

	extern bool particle_properties_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool particle_properties_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void particle_properties_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void particle_properties_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void particle_properties_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t particle_properties_resource_desc;
}
