// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	struct prefab_runtime_t
	{
		u32 reserved = 0;
	};

	struct prefab_internals_t
	{
		chunk_handle32_t source = {};
	};

	class prefab_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('P', 'R', 'F', 'B');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs);
		static bool reload(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);

	private:
		static bool read_source(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, chunk_handle32_t& out_source);
	};

	extern const resource_type_desc_t prefab_resource_desc;
}
