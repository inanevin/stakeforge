// Copyright (c) 2025 Inan Evin

#include "animation.hpp"

namespace sfg
{
	bool animation_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool animation_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void animation_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void animation_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void animation_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t animation_resource_desc = {
		.type				 = resource_type_e::animation,
		.data_size			 = sizeof(animation_data_t),
		.data_alignment		 = alignof(animation_data_t),
		.internals_size		 = sizeof(animation_internals_t),
		.internals_alignment = alignof(animation_internals_t),
		.load				 = animation_load,
		.create_internals	 = animation_create_internals,
		.destroy_internals	 = animation_destroy_internals,
		.unload				 = animation_unload,
		.unload_cpu			 = animation_unload_cpu,
	};
}
