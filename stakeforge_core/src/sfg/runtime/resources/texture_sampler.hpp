// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/descriptions.hpp>

namespace sfg
{
	class istream_t;

	struct texture_sampler_runtime_t
	{
		sampler_desc_t desc = {};
	};

	struct texture_sampler_internals_t
	{
		gfx_sampler_handle sampler	 = {};
		gpu_index_t		   gpu_index = NULL_GPU_INDEX;
	};

	class texture_sampler_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = 0x53504D53;
		static constexpr u32 WIRE_VERSION = 1;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static resource_ready_result_e	 resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t texture_sampler_resource_desc;
}
