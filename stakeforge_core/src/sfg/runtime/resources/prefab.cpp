// Copyright (c) 2025 Inan Evin

#include "prefab.hpp"

namespace sfg
{
	bool prefab_loader_t::load(resource_entry_t&, resource_context_t&, ostream_t&)
	{
		return false;
	}

	bool prefab_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e prefab_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void prefab_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t prefab_resource_desc = {
		.type				 = resource_type_e::prefab,
		.runtime_size		 = sizeof(prefab_runtime_t),
		.runtime_alignment	 = alignof(prefab_runtime_t),
		.internals_size		 = sizeof(prefab_internals_t),
		.internals_alignment = alignof(prefab_internals_t),
		.initial_load_offset = 0,
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.async_load			 = false,
		.load				 = prefab_loader_t::load,
		.load_v2			 = prefab_loader_t::load,
		.create_internals	 = prefab_loader_t::create_internals,
		.destroy_internals	 = prefab_loader_t::destroy_internals,
	};
}
