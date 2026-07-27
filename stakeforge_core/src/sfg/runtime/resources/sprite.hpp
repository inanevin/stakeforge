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
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	enum class sprite_payload_type_e : u8
	{
		ktx2_uastc,
		png,
	};

	SFG_DEFINE_TYPE_ID(sprite_payload_type_e);

	class sprite_loader_t final
	{
	public:
		sprite_loader_t()								   = delete;
		~sprite_loader_t()								   = delete;
		sprite_loader_t(const sprite_loader_t&)			   = delete;
		sprite_loader_t& operator=(const sprite_loader_t&) = delete;

		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'P', 'R', 'T');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct sprite_header_t
	{
		u32						   data_size		= 0;
		vec2u16_t				   size				= vec2u16_t::zero;
		vec2u16_t				   cell_size		= vec2u16_t::zero;
		vec2u16_t				   padding			= vec2u16_t::zero;
		u16						   row_count		= 1;
		u16						   column_count		= 1;
		sprite_payload_type_e	   payload_type		= sprite_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression = texture_ktx2_compression_e::faster;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct sprite_runtime_t
	{
		sprite_header_t header	  = {};
		vec2f_t			uv_size	  = vec2f_t::zero;
		vec2f_t			uv_stride = vec2f_t::zero;
	};

	struct sprite_internals_t
	{
		render_resource_handle_t texture = {};
		render_resource_handle_t staging = {};
	};

	extern const resource_type_desc_t sprite_resource_desc;
}
