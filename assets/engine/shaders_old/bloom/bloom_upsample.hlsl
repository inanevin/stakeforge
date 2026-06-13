// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//     1. Redistributions of source code must retain the above copyright notice, this
//        list of conditions and the following disclaimer.
//  
//     2. Redistributions in binary form must reproduce the above copyright notice,
//        this list of conditions and the following disclaimer in the documentation
//        and/or other materials provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
//  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Upsample mip L -> L-1 with a 3x3 tent filter (COD/SIGGRAPH 2014)

#include "layout_defines_compute.hlsl"
#include "normal.hlsl"
#include "depth.hlsl"

// sfg_rp_constant1 : uint dstWidth   (mip L-1 width)
// sfg_rp_constant2 : uint dstHeight  (mip L-1 height)
// sfg_rp_constant3 : SRV index       (source mip L, RGBA16F/float4)
// sfg_rp_constant4 : UAV index       (dest   mip L-1, RGBA16F/float4)

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

    const uint dstW = (uint)sfg_rp_constant1; // mip L-1 size
    const uint dstH = (uint)sfg_rp_constant2;

    if (p.x >= dstW || p.y >= dstH)
        return;

    // Resources (SRV = mip L, UAV = mip L-1)
    Texture2D<float4>  src = sfg_get_texture<Texture2D<float4> >(sfg_rp_constant3);
    RWTexture2D<float4> dst = sfg_get_texture<RWTexture2D<float4> >(sfg_rp_constant4);

    bloom_params bp = sfg_get_cbv<bloom_params>(sfg_rp_constant0);

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

    dst[p] = float4(up, 1.0);
}