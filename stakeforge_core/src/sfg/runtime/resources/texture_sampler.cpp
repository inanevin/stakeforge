// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"

namespace sfg
{
	bool texture_sampler_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool texture_sampler_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void texture_sampler_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void texture_sampler_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void texture_sampler_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t texture_sampler_resource_desc = {
		.type				 = resource_type_e::texture_sampler,
		.data_size			 = sizeof(texture_sampler_data_t),
		.data_alignment		 = alignof(texture_sampler_data_t),
		.internals_size		 = sizeof(texture_sampler_internals_t),
		.internals_alignment = alignof(texture_sampler_internals_t),
		.load				 = texture_sampler_load,
		.create_internals	 = texture_sampler_create_internals,
		.destroy_internals	 = texture_sampler_destroy_internals,
		.unload				 = texture_sampler_unload,
		.unload_cpu			 = texture_sampler_unload_cpu,
	};
}
