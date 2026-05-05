// Copyright (c) 2025 Inan Evin

#include "animation.hpp"

namespace sfg
{
	bool animation_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e animation_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void animation_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t animation_resource_desc = {
		.type				 = resource_type_e::animation,
		.runtime_size		 = sizeof(animation_runtime_t),
		.runtime_alignment	 = alignof(animation_runtime_t),
		.internals_size		 = sizeof(animation_internals_t),
		.internals_alignment = alignof(animation_internals_t),
		.load				 = animation_loader_t::load,
		.create_internals	 = animation_loader_t::create_internals,
		.destroy_internals	 = animation_loader_t::destroy_internals,
	};
}
