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


#include "layout_defines_compute.hlsl"
#include "normal.hlsl"
#include "depth.hlsl"

// ------------------------------------------------------
// Root/param layout (matches your CPU bindings)
// sfg_rp_constant0 : CBV (unused here)
// sfg_rp_constant1 : uint dstWidth   (mip L+1 width)
// sfg_rp_constant2 : uint dstHeight  (mip L+1 height)
// sfg_rp_constant3 : SRV index       (source mip L,     RGBA16F/float4)
// sfg_rp_constant4 : UAV index       (dest   mip L+1,   RGBA16F/float4)
// sfg_rp_constant5 : uint dstMipLevelIndex (0 for the first downsample level)
// ------------------------------------------------------

struct bloom_params
{
    float filterRadius;   // not used here; reserved
    float _pad0, _pad1, _pad2;
};

SamplerState smp_linear : static_sampler_linear; // linear + clamp

// --- Helpers for Karis average (apply on HDR linear via sRGB luma) ---
float3 PowVec3(float3 v, float p) { return float3(pow(v.x, p), pow(v.y, p), pow(v.z, p)); }

static const float INV_GAMMA = 1.0 / 2.2;

// Convert *linear* HDR to approx sRGB for luma calc
float3 ToSRGB(float3 v)
{
    // ensure non-negative to avoid pow domain issues
    v = max(v, 0.0);
    return PowVec3(v, INV_GAMMA);
}

// BT.601 weights as in the reference snippet (0.299, 0.587, 0.114)
float sRGBToLuma(float3 colSRGB)
{
    return dot(colSRGB, float3(0.299, 0.587, 0.114));
}

// Karis average weight = 1 / (1 + luma*0.25)
// (0.25 factor matches the reference group-mean scaling)
float KarisAverage(float3 linearRGB)
{
    float luma = sRGBToLuma(ToSRGB(linearRGB)) * 0.25;
    return rcp(1.0 + luma);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // Destination pixel in mip L+1 (your downsample target)
    uint2 p = DTid.xy;

    const uint dstW = (uint)sfg_rp_constant1;
    const uint dstH = (uint)sfg_rp_constant2;
    if (p.x >= dstW || p.y >= dstH) return;

    // Resources
    Texture2D<float4>  src = sfg_get_texture<Texture2D<float4> >(sfg_rp_constant3);     // SRV: mip L
    RWTexture2D<float4> dst = sfg_get_texture<RWTexture2D<float4> >(sfg_rp_constant4);  // UAV: mip L+1

    // Which destination mip are we writing? (0 = first downsample level: size/2)
    const uint dstMip = (uint)sfg_rp_constant5;

    // Source (mip L) size is exactly double the destination
    float2 srcSize = float2(dstW * 2.0, dstH * 2.0);
    float2 texel   = 1.0 / srcSize;

    // Map dest pixel (mip L+1) to the center of the corresponding texel in mip L
    float2 baseUV = (float2(p * 2) + 0.5) / srcSize;

    float x = texel.x;
    float y = texel.y;

    // 13-tap neighborhood in mip L (COD/SIGGRAPH)
    float3 a = src.SampleLevel(smp_linear, baseUV + float2(-2*x, +2*y), 0).rgb;
    float3 b = src.SampleLevel(smp_linear, baseUV + float2( 0*x, +2*y), 0).rgb;
    float3 c = src.SampleLevel(smp_linear, baseUV + float2(+2*x, +2*y), 0).rgb;

    float3 d = src.SampleLevel(smp_linear, baseUV + float2(-2*x,  0*y), 0).rgb;
    float3 e = src.SampleLevel(smp_linear, baseUV + float2( 0*x,  0*y), 0).rgb;
    float3 f = src.SampleLevel(smp_linear, baseUV + float2(+2*x,  0*y), 0).rgb;

    float3 g = src.SampleLevel(smp_linear, baseUV + float2(-2*x, -2*y), 0).rgb;
    float3 h = src.SampleLevel(smp_linear, baseUV + float2( 0*x, -2*y), 0).rgb;
    float3 i = src.SampleLevel(smp_linear, baseUV + float2(+2*x, -2*y), 0).rgb;

    float3 j = src.SampleLevel(smp_linear, baseUV + float2(-1*x, +1*y), 0).rgb;
    float3 k = src.SampleLevel(smp_linear, baseUV + float2(+1*x, +1*y), 0).rgb;
    float3 l = src.SampleLevel(smp_linear, baseUV + float2(-1*x, -1*y), 0).rgb;
    float3 m = src.SampleLevel(smp_linear, baseUV + float2(+1*x, -1*y), 0).rgb;

    float3 outRGB;

    // Karis on the first downsample level only (dstMip == 0)
    // We form the same 5 groups of 4 taps as in the reference,
    // scale by their intended group weights divided by 4, then apply Karis.
    if (dstMip == 0)
    {
        float3 g0 = (a + b + d + e) * (0.125 / 4.0);
        float3 g1 = (b + c + e + f) * (0.125 / 4.0);
        float3 g2 = (d + e + g + h) * (0.125 / 4.0);
        float3 g3 = (e + f + h + i) * (0.125 / 4.0);
        float3 g4 = (j + k + l + m) * (0.5   / 4.0);

        g0 *= KarisAverage(g0);
        g1 *= KarisAverage(g1);
        g2 *= KarisAverage(g2);
        g3 *= KarisAverage(g3);
        g4 *= KarisAverage(g4);

        outRGB = g0 + g1 + g2 + g3 + g4;

        // Small floor to avoid denorms/negatives
        outRGB = max(outRGB, 1e-4);
    }
    else
    {
        // Regular energy-preserving weights
        outRGB  = e * 0.125;
        outRGB += (a + c + g + i) * 0.03125;
        outRGB += (b + d + f + h) * 0.0625;
        outRGB += (j + k + l + m) * 0.125;
    }

    dst[p] = float4(outRGB, 1.0);
}
