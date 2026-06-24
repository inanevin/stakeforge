// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	class istream_t;

	struct texture_sampler_runtime_t
	{
		sampler_desc_t desc = {};
	};

	struct texture_sampler_internals_t
	{
		render_resource_handle_t sampler = {};
	};

	class texture_sampler_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'A', 'M', 'P');
		static constexpr u32 WIRE_VERSION = 4;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t texture_sampler_resource_desc;
}
