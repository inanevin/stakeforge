// Copyright (c) 2025 Inan Evin

#include "animation_state_machine.hpp"

namespace sfg
{
	bool animation_state_machine_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool animation_state_machine_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void animation_state_machine_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void animation_state_machine_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void animation_state_machine_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t animation_state_machine_resource_desc = {
		.type				 = resource_type_e::animation_state_machine,
		.data_size			 = sizeof(animation_state_machine_data_t),
		.data_alignment		 = alignof(animation_state_machine_data_t),
		.internals_size		 = sizeof(animation_state_machine_internals_t),
		.internals_alignment = alignof(animation_state_machine_internals_t),
		.load				 = animation_state_machine_load,
		.create_internals	 = animation_state_machine_create_internals,
		.destroy_internals	 = animation_state_machine_destroy_internals,
		.unload				 = animation_state_machine_unload,
		.unload_cpu			 = animation_state_machine_unload_cpu,
	};
}
