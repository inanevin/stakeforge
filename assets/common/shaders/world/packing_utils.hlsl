// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Stakeforge is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  Stakeforge is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.
//
//  As an additional permission under section 7 of GPLv3, the copyright
//  holders grant the Stakeforge Game Linking Exception, version 1.0,
//  in GAME-LINKING-EXCEPTION.md.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

float2 unpack_half2x16(uint packed)
{
    uint lo = packed & 0xFFFFu;
    uint hi = packed >> 16;
    return float2(f16tof32(lo), f16tof32(hi));
}

uint pack_half2x16(float2 v)
{
    uint lo = f32tof16(v.x) & 0xFFFFu;
    uint hi = f32tof16(v.y) & 0xFFFFu;
    return lo | (hi << 16);
}

uint pack_01(float2 val)
{
    uint lo = (uint)round(saturate(val.x) * 65535.0f) & 0xFFFFu;
    uint hi = (uint)round(saturate(val.y) * 65535.0f) & 0xFFFFu;
    return lo | (hi << 16);
}

uint pack_range(float2 v, float max_range)
{
    // Map [0, max_range] -> [0, 65535]
    float2 n = saturate(v / max_range);
    uint2 q = (uint2)round(n * 65535.0f);
    q = min(q, 65535u); // avoid 65536 -> 0 wrap
    return q.x | (q.y << 16);
}

float2 unpack_01(uint val)
{
    uint lo = val & 0xFFFFu;
    uint hi = val >> 16;
    return float2(saturate((float)lo / 65535.0f), saturate((float)hi / 65535.0f));
}

float2 unpack_range(uint p, float max_range)
{
    uint2 q = uint2(p & 0xFFFFu, p >> 16);
    return (float2)q * (max_range / 65535.0f);
}

uint pack_rgba8_unorm(float4 c)
{
    int32_t4 v = (int32_t4)round(saturate(c) * 255.0);
    uint8_t4_packed packed = pack_clamp_u8(v);   // clamps to [0..255]
    return (uint)packed;                         // bitcast, no change
}

float4 unpack_rgba8_unorm(uint p)
{
    uint8_t4_packed packed = (uint8_t4_packed)p; // bitcast
    uint32_t4 v = unpack_u8u32(packed);
    return float4(v) * (1.0 / 255.0);
}