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

#include "format.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	u8 format_get_bpp(format_e fmt)
	{
		switch (fmt)
		{
		case format_e::r8_unorm:
			return 1;
		case format_e::r8g8b8a8_unorm:
		case format_e::r8g8b8a8_srgb:
			return 4;
		case format_e::d16_unorm:
			return 2;
		case format_e::d32_sfloat:
			return 4;
		case format_e::r16g16b16a16_sfloat:
			return 8;
		case format_e::r10g0b10a2_unorm:
			return 4;
		case format_e::r8g8_unorm:
			return 2;
		case format_e::r32_uint:
			return 4;
		default:
			break;
		}

		SFG_ASSERT(false);
		return 0;
	}

	u8 format_get_channels(format_e fmt)
	{
		switch (fmt)
		{
		case format_e::r8_unorm:
			return 1;
		case format_e::r8g8b8a8_unorm:
		case format_e::r8g8b8a8_srgb:
			return 4;
		case format_e::r16g16b16a16_sfloat:
			return 4;
		default:
			break;
		}
		SFG_ASSERT(false);
		return 0;
	}

	bool format_is_block_compressed(format_e fmt)
	{
		switch (fmt)
		{
		case format_e::bc3_block_srgb:
		case format_e::bc3_block_unorm:
		case format_e::bc7_block_srgb:
		case format_e::bc7_block_unorm:
			return true;
		default:
			return false;
		}
	}

	bool format_is_linear(format_e fmt)
	{
		switch (fmt)
		{
		case format_e::b8g8r8a8_srgb:
		case format_e::bc3_block_srgb:
		case format_e::bc7_block_srgb:
		case format_e::r8g8b8a8_srgb:
			return false;
		default:
			return true;
		}

		SFG_ASSERT(false);
		return true;
	}

	u32 format_get_row_pitch(format_e fmt, u16 width)
	{
		if (format_is_block_compressed(fmt))
			return static_cast<u32>((width + 3) / 4) * 16;

		return static_cast<u32>(width) * static_cast<u32>(format_get_bpp(fmt));
	}

	u32 format_get_row_count(format_e fmt, u16 height)
	{
		if (format_is_block_compressed(fmt))
			return static_cast<u32>((height + 3) / 4);

		return height;
	}

	u32 format_get_data_size(format_e fmt, u16 width, u16 height)
	{
		return format_get_row_pitch(fmt, width) * format_get_row_count(fmt, height);
	}

}
