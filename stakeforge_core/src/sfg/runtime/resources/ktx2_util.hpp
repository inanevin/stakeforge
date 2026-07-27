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

#include "texture_payload_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class ostream_t;

	struct ktx2_image_desc_t
	{
		vec2u16_t size		= vec2u16_t::zero;
		format_e  format	= format_e::undefined;
		u8		  mip_count = 0;
	};

	class ktx2_util_t final
	{
	public:
		ktx2_util_t()							   = delete;
		~ktx2_util_t()							   = delete;
		ktx2_util_t(const ktx2_util_t&)			   = delete;
		ktx2_util_t& operator=(const ktx2_util_t&) = delete;

		static bool encode_uastc(span_t<const texture_buffer_t> mips, bool is_linear, texture_ktx2_compression_e compression, const char* source_name, ostream_t& stream);
		static bool decode_uastc(span_t<const u8> data, texture_ktx2_compression_e compression, u64 resource_hash, texture_buffer_t* out_mips, u8 max_mips, ktx2_image_desc_t& out_desc);

	private:
		static void release(texture_buffer_t* mips, u8 mip_count);
	};
}
