// Copyright (c) 2025 Inan Evin

#include "prefab.hpp"

namespace sfg
{
	bool prefab_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool prefab_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void prefab_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void prefab_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void prefab_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t prefab_resource_desc = {
		.type				 = resource_type_e::prefab,
		.data_size			 = sizeof(prefab_data_t),
		.data_alignment		 = alignof(prefab_data_t),
		.internals_size		 = sizeof(prefab_internals_t),
		.internals_alignment = alignof(prefab_internals_t),
		.load				 = prefab_load,
		.create_internals	 = prefab_create_internals,
		.destroy_internals	 = prefab_destroy_internals,
		.unload				 = prefab_unload,
		.unload_cpu			 = prefab_unload_cpu,
	};
}
