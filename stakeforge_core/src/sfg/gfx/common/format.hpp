/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#pragma once

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>

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

	SFG_DEFINE_TYPE_ID(format_e);

	struct format_reflection_t
	{
		format_reflection_t();
	};

	inline format_reflection_t g_reflect_format;
}
