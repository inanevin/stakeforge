// Copyright (c) 2025 Inan Evin

#include "skeleton.hpp"

namespace sfg
{
	bool skeleton_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e skeleton_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void skeleton_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t skeleton_resource_desc = {
		.type				 = resource_type_e::skeleton,
		.runtime_size		 = sizeof(skeleton_runtime_t),
		.runtime_alignment	 = alignof(skeleton_runtime_t),
		.internals_size		 = sizeof(skeleton_internals_t),
		.internals_alignment = alignof(skeleton_internals_t),
		.load				 = skeleton_loader_t::load,
		.create_internals	 = skeleton_loader_t::create_internals,
		.destroy_internals	 = skeleton_loader_t::destroy_internals,
	};
}
