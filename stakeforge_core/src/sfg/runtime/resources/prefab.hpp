// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct prefab_runtime_t
	{
		u32 reserved = 0;
	};

	struct prefab_internals_t
	{
		u32 reserved = 0;
	};

	class prefab_loader_t
	{
	public:
		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static bool						 load(resource_entry_t& entry, resource_context_t& ctx, ostream_t& stream);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t prefab_resource_desc;
}
