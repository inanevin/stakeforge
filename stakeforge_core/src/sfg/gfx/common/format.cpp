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
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/io/assert.hpp>

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
		case format_e::r16g16_sfloat:
			return 4;
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
		case format_e::r16g16_sfloat:
			return 2;
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

namespace sfg
{
	format_reflection_t::format_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "format_e",
			.display_name = "Format",
			.fields =
				{
					{.name = "undefined", .display_name = "Undefined"},
					{.name = "r8_sint", .display_name = "R8 SInt"},
					{.name = "r8_uint", .display_name = "R8 UInt"},
					{.name = "r8", .display_name = "R8 UNorm"},
					{.name = "r8_snorm", .display_name = "R8 SNorm"},
					{.name = "r8g8_sint", .display_name = "R8G8 SInt"},
					{.name = "r8g8_uint", .display_name = "R8G8 UInt"},
					{.name = "r8g8_unorm", .display_name = "R8G8 UNorm"},
					{.name = "r8g8_snorm", .display_name = "R8G8 SNorm"},
					{.name = "r8g8b8a8_sint", .display_name = "R8G8B8A8 SInt"},
					{.name = "r8g8b8a8_uint", .display_name = "R8G8B8A8 UInt"},
					{.name = "r8g8b8a8_unorm", .display_name = "R8G8B8A8 UNorm"},
					{.name = "r8g8b8a8_snorm", .display_name = "R8G8B8A8 SNorm"},
					{.name = "r8g8b8a8_srgb", .display_name = "R8G8B8A8 SRGB"},
					{.name = "b8g8r8a8_unorm", .display_name = "B8G8R8A8 UNorm"},
					{.name = "b8g8r8a8_srgb", .display_name = "B8G8R8A8 SRGB"},
					{.name = "r16_sint", .display_name = "R16 SInt"},
					{.name = "r16_uint", .display_name = "R16 UInt"},
					{.name = "r16_unorm", .display_name = "R16 UNorm"},
					{.name = "r16_snorm", .display_name = "R16 SNorm"},
					{.name = "r16_sfloat", .display_name = "R16 SFloat"},
					{.name = "r16g16_sint", .display_name = "R16G16 SInt"},
					{.name = "r16g16_uint", .display_name = "R16G16 UInt"},
					{.name = "r16g16_unorm", .display_name = "R16G16 UNorm"},
					{.name = "r16g16_snorm", .display_name = "R16G16 SNorm"},
					{.name = "r16g16_sfloat", .display_name = "R16G16 SFloat"},
					{.name = "r16g16b16a16_sint", .display_name = "R16G16B16A16 SInt"},
					{.name = "r16g16b16a16_uint", .display_name = "R16G16B16A16 UInt"},
					{.name = "r16g16b16a16_unorm", .display_name = "R16G16B16A16 UNorm"},
					{.name = "r16g16b16a16_snorm", .display_name = "R16G16B16A16 SNorm"},
					{.name = "r16g16b16a16_sfloat", .display_name = "R16G16B16A16 SFloat"},
					{.name = "r32_sint", .display_name = "R32 SInt"},
					{.name = "r32_uint", .display_name = "R32 UInt"},
					{.name = "r32_sfloat", .display_name = "R32 SFloat"},
					{.name = "r32g32_sint", .display_name = "R32G32 SInt"},
					{.name = "r32g32_uint", .display_name = "R32G32 UInt"},
					{.name = "r32g32_sfloat", .display_name = "R32G32 SFloat"},
					{.name = "r32g32b32_sfloat", .display_name = "R32G32B32 SFloat"},
					{.name = "r32g32b32_sint", .display_name = "R32G32B32 SInt"},
					{.name = "r32g32b32_uint", .display_name = "R32G32B32 UInt"},
					{.name = "r32g32b32a32_sint", .display_name = "R32G32B32A32 SInt"},
					{.name = "r32g32b32a32_uint", .display_name = "R32G32B32A32 UInt"},
					{.name = "r32g32b32a32_sfloat", .display_name = "R32G32B32A32 SFloat"},
					{.name = "d32_sfloat", .display_name = "D32 SFloat"},
					{.name = "d24_unorm_s8_uint", .display_name = "D24 UNorm S8 UInt"},
					{.name = "d16_unorm", .display_name = "D16 UNorm"},
					{.name = "r11g11b10_sfloat", .display_name = "R11G11B10 SFloat"},
					{.name = "r10g0b10a2_int", .display_name = "R10G0B10A2 Int"},
					{.name = "r10g0b10a2_unorm", .display_name = "R10G10B10A2 UNorm"},
					{.name = "bc3_block_srgb", .display_name = "BC3 Block SRGB"},
					{.name = "bc3_block_unorm", .display_name = "BC3 Block UNorm"},
					{.name = "bc7_block_srgb", .display_name = "BC7 Block SRGB"},
					{.name = "bc7_block_unorm", .display_name = "BC7 Block UNorm"},
				},
			.type_id   = type_id_t<format_e>::value,
			.size	   = sizeof(format_e),
			.alignment = alignof(format_e),
			.flags	   = reflected_type_flag_enum,
		});
	}
}
