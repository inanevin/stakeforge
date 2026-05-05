// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"

namespace sfg
{
	bool texture_sampler_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e texture_sampler_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void texture_sampler_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t texture_sampler_resource_desc = {
		.type				 = resource_type_e::texture_sampler,
		.runtime_size		 = sizeof(texture_sampler_runtime_t),
		.runtime_alignment	 = alignof(texture_sampler_runtime_t),
		.internals_size		 = sizeof(texture_sampler_internals_t),
		.internals_alignment = alignof(texture_sampler_internals_t),
		.load				 = texture_sampler_loader_t::load,
		.create_internals	 = texture_sampler_loader_t::create_internals,
		.destroy_internals	 = texture_sampler_loader_t::destroy_internals,
	};
}
