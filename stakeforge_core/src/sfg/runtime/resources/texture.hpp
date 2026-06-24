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
#include "texture_payload_type.hpp"
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <cstddef>

namespace sfg
{
	class istream_t;

	class texture_loader_t
	{
	public:
		static constexpr u8	 MAX_MIPS	  = 16;
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('T', 'E', 'X', 'R');
		static constexpr u32 WIRE_VERSION = 9;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct texture_mip_header_t
	{
		size_t	  byte_offset = 0;
		vec2u16_t size		  = vec2u16_t::zero;
		u8		  bpp		  = 0;
	};

	struct texture_header_t
	{
		texture_mip_header_t mips[texture_loader_t::MAX_MIPS] = {};
		vec2u16_t			 size							  = vec2u16_t::zero;
		u8					 bpp							  = 0;
		u8					 mip_count						  = 0;
		u8					 is_linear						  = 0;
	};

	struct texture_runtime_t
	{
		texture_buffer_t		   mips[texture_loader_t::MAX_MIPS] = {};
		format_e				   texture_format					= format_e::undefined;
		texture_payload_type_e	   payload_type						= texture_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression					= texture_ktx2_compression_e::default_quality;
		u8						   channels							= 0;
		u8						   mip_count						= 0;
		u8						   is_linear						= 0;
	};

	struct texture_internals_t
	{
		render_resource_handle_t texture = {};
		render_resource_handle_t staging = {};
	};

	extern const resource_type_desc_t texture_resource_desc;
}
