/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	class cubemap_loader_t final
	{
	public:
		static inline constexpr u32 WIRE_MAGIC		 = make_resource_wire_magic('C', 'U', 'B', 'E');
		static inline constexpr u32 WIRE_VERSION	 = 1;
		static inline constexpr u8	FACE_COUNT		 = 6;
		static inline constexpr u8	MAX_MIPS		 = 16;
		static inline constexpr u8	MAX_SUBRESOURCES = FACE_COUNT * MAX_MIPS;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct cubemap_runtime_t
	{
		vec2u16_t size		= vec2u16_t::zero;
		format_e  format	= format_e::undefined;
		u8		  mip_count = 0;
	};

	struct cubemap_internals_t
	{
		render_resource_handle_t staging[cubemap_loader_t::FACE_COUNT] = {};
		render_resource_handle_t texture							   = {};
	};

	extern const resource_type_desc_t cubemap_resource_desc;
}
