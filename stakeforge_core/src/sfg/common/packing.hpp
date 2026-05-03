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

#include "size_definitions.hpp"
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	namespace packing
	{
		inline u16 float_to_half(f32 f)
		{
			u32 x;
			SFG_MEMCPY(&x, &f, sizeof(x));
			u32 sign = (x >> 16) & 0x8000u;
			u32 mant = x & 0x007FFFFFu;
			i32 exp	 = i32((x >> 23) & 0xFF) - 127 + 15; // re-bias

			if (exp <= 0)
			{
				// subnormal or underflow to zero
				if (exp < -10)
					return (u16)sign; // ?0
				// add implicit 1, shift to align into 10-bit mantissa
				mant |= 0x00800000u;
				u32 t = mant >> (1 - exp + 13); // 13 = 23-10
				// round to nearest
				if ((mant >> (1 - exp + 12)) & 1u)
					t += 1;
				return (u16)(sign | t);
			}
			else if (exp >= 31)
			{
				// overflow -> inf (or keep NaN)
				if (mant == 0)
					return (u16)(sign | 0x7C00u); // inf
				// NaN, preserve payload top bit
				return (u16)(sign | 0x7C00u | (mant >> 13) | ((mant >> 13) == 0));
			}
			else
			{
				// normal
				u32 h = (u32(exp) << 10) | (mant >> 13);
				// round to nearest-even
				if (mant & 0x00001000u)
					h += 1;
				return (u16)(sign | h);
			}
		}

		inline u32 pack_half2x16(f32 x, f32 y)
		{
			const u16 hx = float_to_half(x);
			const u16 hy = float_to_half(y);
			return (u32(hy) << 16) | u32(hx);
		}

		inline u8 pack_unorm8(f32 x, f32 range)
		{
			f32 u = math::clamp(x / range, 0.0f, 1.0f);
			return (u8)math::lround(u * 255.0f);
		}

		inline int8_t pack_snorm8(f32 x, f32 range)
		{
			f32 s = math::clamp(x / range, -1.0f, 1.0f);
			// Map [-1,1] to [-128,127]; clamp to avoid -128 edge cases
			int v = (int)math::lround(s * 127.0f);
			return (int8_t)math::clamp(v, -128, 127);
		}

		inline uint32_t pack4_snorm8(f32 tx, f32 ty, f32 ox, f32 oy, f32 tilingRange, f32 offsetRange)
		{
			uint32_t b0 = (u8)pack_snorm8(tx, tilingRange);
			uint32_t b1 = (u8)pack_snorm8(ty, tilingRange);
			uint32_t b2 = (u8)pack_snorm8(ox, offsetRange);
			uint32_t b3 = (u8)pack_snorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}

		inline uint32_t pack4_unorm8(f32 tx, f32 ty, f32 ox, f32 oy, f32 tilingRange, f32 offsetRange)
		{
			uint32_t b0 = pack_unorm8(tx, tilingRange);
			uint32_t b1 = pack_unorm8(ty, tilingRange);
			uint32_t b2 = pack_unorm8(ox, offsetRange);
			uint32_t b3 = pack_unorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}

		inline uint32_t pack4_unorm8(f32 tx, f32 ty, f32 ox, f32 oy)
		{
			uint32_t b0 = pack_unorm8(tx, 1.0f);
			uint32_t b1 = pack_unorm8(ty, 1.0f);
			uint32_t b2 = pack_unorm8(ox, 1.0f);
			uint32_t b3 = pack_unorm8(oy, 1.0f);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}
	}
}
