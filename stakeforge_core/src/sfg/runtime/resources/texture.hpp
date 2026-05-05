// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	class istream_t;

	class texture_loader_t
	{
	public:
		static constexpr u8	 MAX_MIPS	  = 16;
		static constexpr u32 WIRE_MAGIC	  = 0x53465458;
		static constexpr u32 WIRE_VERSION = 3;

		static bool						   load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e   create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static complete_internals_result_e complete_internals(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion);
		static void						   destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct texture_runtime_mip_t
	{
		u32 offset = 0;
		u32 size   = 0;
		u32 width  = 0;
		u32 height = 0;
	};

	struct texture_runtime_t
	{
		u32					  width							   = 0;
		u32					  height						   = 0;
		u8					  channels						   = 0;
		u8					  mip_count						   = 0;
		u8					  is_linear						   = 0;
		texture_runtime_mip_t mips[texture_loader_t::MAX_MIPS] = {};
	};

	struct texture_internals_t
	{
		gfx_texture_handle texture = {};
	};

	extern const resource_type_desc_t texture_resource_desc;
}
