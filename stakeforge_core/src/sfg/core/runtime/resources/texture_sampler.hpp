// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/runtime/resources/common_resources.hpp"
#include "core/gfx/common/descriptions.hpp"
#include "core/runtime/render/render_resource_handle.hpp"

namespace sfg
{
	struct texture_sampler_runtime_t
	{
		sampler_desc_t desc = {};
	};

	struct texture_sampler_internals_t
	{
		render_resource_handle_t sampler = {};
	};

	class SFG_API texture_sampler_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'A', 'M', 'P');
		static constexpr u32 WIRE_VERSION = 0;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern SFG_API const resource_type_desc_t texture_sampler_resource_desc;
}
