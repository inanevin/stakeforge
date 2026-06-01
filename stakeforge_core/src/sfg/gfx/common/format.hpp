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

#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	enum class format_e : u8
	{
		undefined = 0,

		// 8 bit
		r8_sint,
		r8_uint,
		r8_unorm,
		r8_snorm,

		r8g8_sint,
		r8g8_uint,
		r8g8_unorm,
		r8g8_snorm,

		r8g8b8a8_sint,
		r8g8b8a8_uint,
		r8g8b8a8_unorm,
		r8g8b8a8_snorm,
		r8g8b8a8_srgb,

		b8g8r8a8_unorm,
		b8g8r8a8_srgb,

		// 16 bit
		r16_sint,
		r16_uint,
		r16_unorm,
		r16_snorm,
		r16_sfloat,

		r16g16_sint,
		r16g16_uint,
		r16g16_unorm,
		r16g16_snorm,
		r16g16_sfloat,

		r16g16b16a16_sint,
		r16g16b16a16_uint,
		r16g16b16a16_unorm,
		r16g16b16a16_snorm,
		r16g16b16a16_sfloat,

		// 32 bit
		r32_sint,
		r32_uint,
		r32_sfloat,

		r32g32_sint,
		r32g32_uint,
		r32g32_sfloat,

		r32g32b32_sfloat,
		r32g32b32_sint,
		r32g32b32_uint,

		r32g32b32a32_sint,
		r32g32b32a32_uint,
		r32g32b32a32_sfloat,

		// depth-stencil
		d32_sfloat,
		d24_unorm_s8_uint,
		d16_unorm,

		// misc
		r11g11b10_sfloat,
		r10g0b10a2_int,
		r10g0b10a2_unorm,
		bc3_block_srgb,
		bc3_block_unorm,
		bc7_block_srgb,
		bc7_block_unorm,
		format_max,
	};

	u8	 format_get_bpp(format_e fmt);
	u8	 format_get_channels(format_e fmt);
	bool format_is_block_compressed(format_e fmt);
	bool format_is_linear(format_e fmt);
	u32	 format_get_row_pitch(format_e fmt, u16 width);
	u32	 format_get_row_count(format_e fmt, u16 height);
	u32	 format_get_data_size(format_e fmt, u16 width, u16 height);

	void to_json(nlohmann::json& j, const format_e& f);
	void from_json(const nlohmann::json& j, format_e& f);
}
