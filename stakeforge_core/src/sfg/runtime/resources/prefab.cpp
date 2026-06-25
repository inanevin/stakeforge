// Copyright (c) 2025 Inan Evin

#include "prefab.hpp"

#include <sfg/io/log.hpp>

namespace sfg
{
	bool prefab_loader_t::load(resource_entry_t&, resource_context_t&, resource_file_system_t&)
	{
		SFG_ERR("prefab loading is not implemented");
		return false;
	}

	void prefab_loader_t::unload(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t prefab_resource_desc = {
		.type				 = resource_type_e::prefab,
		.runtime_size		 = sizeof(prefab_runtime_t),
		.runtime_alignment	 = alignof(prefab_runtime_t),
		.internals_size		 = sizeof(prefab_internals_t),
		.internals_alignment = alignof(prefab_internals_t),
		.use_async_load		 = false,
		.load				 = prefab_loader_t::load,
		.unload				 = prefab_loader_t::unload,
	};
}
