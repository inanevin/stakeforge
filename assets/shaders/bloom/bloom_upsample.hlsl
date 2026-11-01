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

// Upsample mip L -> L-1 with a 3x3 tent filter (COD/SIGGRAPH 2014)
// Jorge Jimenez - https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#include "layout_defines.hlsl"
#include "normal.hlsl"
#include "depth.hlsl"

// sfg_constant_rp1 : uint dstWidth   (mip L-1 width)
// sfg_constant_rp2 : uint dstHeight  (mip L-1 height)
// sfg_constant_rp3 : SRV index       (source mip L, RGBA16F/float4)
// sfg_constant_rp4 : SRV index       (downsample mip L-1, RGBA16F/float4)
// sfg_constant_rp5 : UAV index       (dest   mip L-1, RGBA16F/float4)
// sfg_constant_rp6 : uint            (whether the downsample input exists)

struct bloom_params
{
    float filterRadius;   // in texture coordinates (e.g., k / min(srcW, srcH))
    float _pad0, _pad1, _pad2;
};

SamplerState smp_linear : static_sampler_linear; // linear + clamp

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 p = DTid.xy;

    const uint dstW = (uint)sfg_constant_rp1; // mip L-1 size
    const uint dstH = (uint)sfg_constant_rp2;

    if (p.x >= dstW || p.y >= dstH)
        return;

    // Resources (SRV = mip L, UAV = mip L-1)
    Texture2D<float4>  src = sfg_get_texture<Texture2D<float4> >(sfg_constant_rp3);
    RWTexture2D<float4> dst = sfg_get_texture<RWTexture2D<float4> >(sfg_constant_rp5);

    bloom_params bp = sfg_get_cbv<bloom_params>(sfg_constant_rp0);

    // Normalized UV at the center of the destination pixel (mip L-1)
    float2 dstSize = float2(dstW, dstH);
    float2 uv = (float2(p) + 0.5) / dstSize;

    // Tent kernel radius in texture coordinates (same across mips)
    float filter = bp.filterRadius;
    float x = filter;
    float y = filter;

    // 3x3 taps around uv (sampling from the lower-res mip L)
    // a - b - c
    // d - e - f
    // g - h - i
    float3 a = src.SampleLevel(smp_linear, uv + float2(-x, +y), 0).rgb;
    float3 b = src.SampleLevel(smp_linear, uv + float2( 0,   +y), 0).rgb;
    float3 c = src.SampleLevel(smp_linear, uv + float2(+x, +y), 0).rgb;

    float3 d = src.SampleLevel(smp_linear, uv + float2(-x,  0), 0).rgb;
    float3 e = src.SampleLevel(smp_linear, uv,                     0).rgb;
    float3 f = src.SampleLevel(smp_linear, uv + float2(+x,  0), 0).rgb;

    float3 g = src.SampleLevel(smp_linear, uv + float2(-x, -y), 0).rgb;
    float3 h = src.SampleLevel(smp_linear, uv + float2( 0,   -y), 0).rgb;
    float3 i = src.SampleLevel(smp_linear, uv + float2(+x, -y), 0).rgb;

    // 3x3 tent weights:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    float3 up = e*4.0
              + (b + d + f + h)*2.0
              + (a + c + g + i);
    up *= 1.0 / 16.0;

    if (sfg_constant_rp6 != 0)
    {
        Texture2D<float4> downsample = sfg_get_texture<Texture2D<float4> >(sfg_constant_rp4);
        up += downsample.SampleLevel(smp_linear, uv, 0).rgb;
    }

    dst[p] = float4(up, 1.0);
}
