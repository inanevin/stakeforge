// Copyright (c) 2025 Inan Evin

#include "animation_state_machine.hpp"

namespace sfg
{
	bool animation_state_machine_loader_t::load(resource_entry_t&, resource_context_t&, istream_t&)
	{
		return false;
	}

	void animation_state_machine_loader_t::unload(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t animation_state_machine_resource_desc = {
		.type				 = resource_type_e::animation_state_machine,
		.runtime_size		 = sizeof(animation_state_machine_runtime_t),
		.runtime_alignment	 = alignof(animation_state_machine_runtime_t),
		.internals_size		 = sizeof(animation_state_machine_internals_t),
		.internals_alignment = alignof(animation_state_machine_internals_t),
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.use_async_load		 = false,
		.load				 = animation_state_machine_loader_t::load,
		.unload				 = animation_state_machine_loader_t::unload,
	};
}
