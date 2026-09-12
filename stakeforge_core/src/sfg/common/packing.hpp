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

#include "size_definitions.hpp"
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	class packing_t
	{
	public:
		static inline u16 float_to_half(f32 f)
		{
			u32 x;
			SFG_MEMCPY(&x, &f, sizeof(x));
			u32 sign = (x >> 16) & 0x8000u;
			u32 mant = x & 0x007FFFFFu;
			i32 exp	 = i32((x >> 23) & 0xFF) - 127 + 15;

			if (exp <= 0)
			{
				if (exp < -10)
					return (u16)sign;
				mant |= 0x00800000u;
				u32 t = mant >> (1 - exp + 13);
				if ((mant >> (1 - exp + 12)) & 1u)
					t += 1;
				return (u16)(sign | t);
			}
			else if (exp >= 31)
			{
				if (mant == 0)
					return (u16)(sign | 0x7C00u);
				return (u16)(sign | 0x7C00u | (mant >> 13) | ((mant >> 13) == 0));
			}
			else
			{
				u32 h = (u32(exp) << 10) | (mant >> 13);
				if (mant & 0x00001000u)
					h += 1;
				return (u16)(sign | h);
			}
		}

		static inline u32 pack_half2x16(f32 x, f32 y)
		{
			const u16 hx = float_to_half(x);
			const u16 hy = float_to_half(y);
			return (u32(hy) << 16) | u32(hx);
		}

		static inline u8 pack_unorm8(f32 x, f32 range)
		{
			f32 u = math::clamp(x / range, 0.0f, 1.0f);
			return (u8)math::lround(u * 255.0f);
		}

		static inline int8_t pack_snorm8(f32 x, f32 range)
		{
			f32 s = math::clamp(x / range, -1.0f, 1.0f);
			int v = (int)math::lround(s * 127.0f);
			return (int8_t)math::clamp(v, -128, 127);
		}

		static inline uint32_t pack4_snorm8(f32 tx, f32 ty, f32 ox, f32 oy, f32 tilingRange, f32 offsetRange)
		{
			uint32_t b0 = (u8)pack_snorm8(tx, tilingRange);
			uint32_t b1 = (u8)pack_snorm8(ty, tilingRange);
			uint32_t b2 = (u8)pack_snorm8(ox, offsetRange);
			uint32_t b3 = (u8)pack_snorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}

		static inline uint32_t pack4_unorm8(f32 tx, f32 ty, f32 ox, f32 oy, f32 tilingRange, f32 offsetRange)
		{
			uint32_t b0 = pack_unorm8(tx, tilingRange);
			uint32_t b1 = pack_unorm8(ty, tilingRange);
			uint32_t b2 = pack_unorm8(ox, offsetRange);
			uint32_t b3 = pack_unorm8(oy, offsetRange);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}

		static inline uint32_t pack4_unorm8(f32 tx, f32 ty, f32 ox, f32 oy)
		{
			uint32_t b0 = pack_unorm8(tx, 1.0f);
			uint32_t b1 = pack_unorm8(ty, 1.0f);
			uint32_t b2 = pack_unorm8(ox, 1.0f);
			uint32_t b3 = pack_unorm8(oy, 1.0f);
			return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
		}
	};
}
