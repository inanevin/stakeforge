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
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
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
	namespace
	{
		static const reflected_enum_value_desc_t format_values[] = {
			{.name = "undefined", .display_name = "Undefined", .value = static_cast<i64>(format_e::undefined)},
			{.name = "r8_sint", .display_name = "R8 SInt", .value = static_cast<i64>(format_e::r8_sint)},
			{.name = "r8_uint", .display_name = "R8 UInt", .value = static_cast<i64>(format_e::r8_uint)},
			{.name = "r8", .display_name = "R8 UNorm", .value = static_cast<i64>(format_e::r8_unorm)},
			{.name = "r8_snorm", .display_name = "R8 SNorm", .value = static_cast<i64>(format_e::r8_snorm)},
			{.name = "r8g8_sint", .display_name = "R8G8 SInt", .value = static_cast<i64>(format_e::r8g8_sint)},
			{.name = "r8g8_uint", .display_name = "R8G8 UInt", .value = static_cast<i64>(format_e::r8g8_uint)},
			{.name = "r8g8_unorm", .display_name = "R8G8 UNorm", .value = static_cast<i64>(format_e::r8g8_unorm)},
			{.name = "r8g8_snorm", .display_name = "R8G8 SNorm", .value = static_cast<i64>(format_e::r8g8_snorm)},
			{.name = "r8g8b8a8_sint", .display_name = "R8G8B8A8 SInt", .value = static_cast<i64>(format_e::r8g8b8a8_sint)},
			{.name = "r8g8b8a8_uint", .display_name = "R8G8B8A8 UInt", .value = static_cast<i64>(format_e::r8g8b8a8_uint)},
			{.name = "r8g8b8a8_unorm", .display_name = "R8G8B8A8 UNorm", .value = static_cast<i64>(format_e::r8g8b8a8_unorm)},
			{.name = "r8g8b8a8_snorm", .display_name = "R8G8B8A8 SNorm", .value = static_cast<i64>(format_e::r8g8b8a8_snorm)},
			{.name = "r8g8b8a8_srgb", .display_name = "R8G8B8A8 SRGB", .value = static_cast<i64>(format_e::r8g8b8a8_srgb)},
			{.name = "b8g8r8a8_unorm", .display_name = "B8G8R8A8 UNorm", .value = static_cast<i64>(format_e::b8g8r8a8_unorm)},
			{.name = "b8g8r8a8_srgb", .display_name = "B8G8R8A8 SRGB", .value = static_cast<i64>(format_e::b8g8r8a8_srgb)},
			{.name = "r16_sint", .display_name = "R16 SInt", .value = static_cast<i64>(format_e::r16_sint)},
			{.name = "r16_uint", .display_name = "R16 UInt", .value = static_cast<i64>(format_e::r16_uint)},
			{.name = "r16_unorm", .display_name = "R16 UNorm", .value = static_cast<i64>(format_e::r16_unorm)},
			{.name = "r16_snorm", .display_name = "R16 SNorm", .value = static_cast<i64>(format_e::r16_snorm)},
			{.name = "r16_sfloat", .display_name = "R16 SFloat", .value = static_cast<i64>(format_e::r16_sfloat)},
			{.name = "r16g16_sint", .display_name = "R16G16 SInt", .value = static_cast<i64>(format_e::r16g16_sint)},
			{.name = "r16g16_uint", .display_name = "R16G16 UInt", .value = static_cast<i64>(format_e::r16g16_uint)},
			{.name = "r16g16_unorm", .display_name = "R16G16 UNorm", .value = static_cast<i64>(format_e::r16g16_unorm)},
			{.name = "r16g16_snorm", .display_name = "R16G16 SNorm", .value = static_cast<i64>(format_e::r16g16_snorm)},
			{.name = "r16g16_sfloat", .display_name = "R16G16 SFloat", .value = static_cast<i64>(format_e::r16g16_sfloat)},
			{.name = "r16g16b16a16_sint", .display_name = "R16G16B16A16 SInt", .value = static_cast<i64>(format_e::r16g16b16a16_sint)},
			{.name = "r16g16b16a16_uint", .display_name = "R16G16B16A16 UInt", .value = static_cast<i64>(format_e::r16g16b16a16_uint)},
			{.name = "r16g16b16a16_unorm", .display_name = "R16G16B16A16 UNorm", .value = static_cast<i64>(format_e::r16g16b16a16_unorm)},
			{.name = "r16g16b16a16_snorm", .display_name = "R16G16B16A16 SNorm", .value = static_cast<i64>(format_e::r16g16b16a16_snorm)},
			{.name = "r16g16b16a16_sfloat", .display_name = "R16G16B16A16 SFloat", .value = static_cast<i64>(format_e::r16g16b16a16_sfloat)},
			{.name = "r32_sint", .display_name = "R32 SInt", .value = static_cast<i64>(format_e::r32_sint)},
			{.name = "r32_uint", .display_name = "R32 UInt", .value = static_cast<i64>(format_e::r32_uint)},
			{.name = "r32_sfloat", .display_name = "R32 SFloat", .value = static_cast<i64>(format_e::r32_sfloat)},
			{.name = "r32g32_sint", .display_name = "R32G32 SInt", .value = static_cast<i64>(format_e::r32g32_sint)},
			{.name = "r32g32_uint", .display_name = "R32G32 UInt", .value = static_cast<i64>(format_e::r32g32_uint)},
			{.name = "r32g32_sfloat", .display_name = "R32G32 SFloat", .value = static_cast<i64>(format_e::r32g32_sfloat)},
			{.name = "r32g32b32_sfloat", .display_name = "R32G32B32 SFloat", .value = static_cast<i64>(format_e::r32g32b32_sfloat)},
			{.name = "r32g32b32_sint", .display_name = "R32G32B32 SInt", .value = static_cast<i64>(format_e::r32g32b32_sint)},
			{.name = "r32g32b32_uint", .display_name = "R32G32B32 UInt", .value = static_cast<i64>(format_e::r32g32b32_uint)},
			{.name = "r32g32b32a32_sint", .display_name = "R32G32B32A32 SInt", .value = static_cast<i64>(format_e::r32g32b32a32_sint)},
			{.name = "r32g32b32a32_uint", .display_name = "R32G32B32A32 UInt", .value = static_cast<i64>(format_e::r32g32b32a32_uint)},
			{.name = "r32g32b32a32_sfloat", .display_name = "R32G32B32A32 SFloat", .value = static_cast<i64>(format_e::r32g32b32a32_sfloat)},
			{.name = "d32_sfloat", .display_name = "D32 SFloat", .value = static_cast<i64>(format_e::d32_sfloat)},
			{.name = "d24_unorm_s8_uint", .display_name = "D24 UNorm S8 UInt", .value = static_cast<i64>(format_e::d24_unorm_s8_uint)},
			{.name = "d16_unorm", .display_name = "D16 UNorm", .value = static_cast<i64>(format_e::d16_unorm)},
			{.name = "r11g11b10_sfloat", .display_name = "R11G11B10 SFloat", .value = static_cast<i64>(format_e::r11g11b10_sfloat)},
			{.name = "r10g0b10a2_int", .display_name = "R10G0B10A2 Int", .value = static_cast<i64>(format_e::r10g0b10a2_int)},
			{.name = "r10g0b10a2_unorm", .display_name = "R10G10B10A2 UNorm", .value = static_cast<i64>(format_e::r10g0b10a2_unorm)},
			{.name = "r10g10b10a2_unorm", .display_name = "R10G10B10A2 UNorm", .value = static_cast<i64>(format_e::r10g0b10a2_unorm)},
			{.name = "bc3_block_srgb", .display_name = "BC3 Block SRGB", .value = static_cast<i64>(format_e::bc3_block_srgb)},
			{.name = "bc3_block_unorm", .display_name = "BC3 Block UNorm", .value = static_cast<i64>(format_e::bc3_block_unorm)},
			{.name = "bc7_block_srgb", .display_name = "BC7 Block SRGB", .value = static_cast<i64>(format_e::bc7_block_srgb)},
			{.name = "bc7_block_unorm", .display_name = "BC7 Block UNorm", .value = static_cast<i64>(format_e::bc7_block_unorm)},
		};
	}

	format_reflection_t::format_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<format_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = format_values, .size = std::size(format_values)},
			.name		 = "format_e",
			.type_id	 = type_id_t<format_e>::value,
			.size		 = sizeof(format_e),
			.alignment	 = alignof(format_e),
		});
	}
}
