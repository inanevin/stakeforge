// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>

namespace sfg
{
	class istream_t;

	class texture_loader_t
	{
	public:
		static constexpr u8	 MAX_MIPS	  = 16;
		static constexpr u32 WIRE_MAGIC	  = 0x53465458;
		static constexpr u32 WIRE_VERSION = 3;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static resource_ready_result_e	 resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct texture_runtime_t
	{
		u8				 channels						  = 0;
		u8				 mip_count						  = 0;
		u8				 is_linear						  = 0;
		texture_buffer_t mips[texture_loader_t::MAX_MIPS] = {};
	};

	struct texture_internals_t
	{
		gfx_texture_handle	texture		  = {};
		gfx_resource_handle staging		  = {};
		u8					pending_count = 0;
		u8					had_failure	  = 0;
	};

	extern const resource_type_desc_t texture_resource_desc;
}
