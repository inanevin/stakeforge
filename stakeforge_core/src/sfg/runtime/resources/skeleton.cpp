// Copyright (c) 2025 Inan Evin

#include "skeleton.hpp"

namespace sfg
{
	bool skeleton_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool skeleton_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void skeleton_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void skeleton_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void skeleton_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t skeleton_resource_desc = {
		.type				 = resource_type_e::skeleton,
		.data_size			 = sizeof(skeleton_data_t),
		.data_alignment		 = alignof(skeleton_data_t),
		.internals_size		 = sizeof(skeleton_internals_t),
		.internals_alignment = alignof(skeleton_internals_t),
		.load				 = skeleton_load,
		.create_internals	 = skeleton_create_internals,
		.destroy_internals	 = skeleton_destroy_internals,
		.unload				 = skeleton_unload,
		.unload_cpu			 = skeleton_unload_cpu,
	};
}
