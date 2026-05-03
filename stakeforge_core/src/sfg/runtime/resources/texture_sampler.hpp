// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct texture_sampler_data_t
	{
		u32 reserved = 0;
	};

	struct texture_sampler_internals_t
	{
		u32 reserved = 0;
	};

	extern bool texture_sampler_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool texture_sampler_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_sampler_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_sampler_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_sampler_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t texture_sampler_resource_desc;
}
