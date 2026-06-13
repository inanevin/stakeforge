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

#include "layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------
struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv  : uv0;
};

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
// tonemap_mode: 0 = ACES, 1 = Reinhard, 2 = None (bypass)
// wb_temp:  -1..+1   (negative = cooler, positive = warmer)
// wb_tint:  -1..+1   (negative = magenta, positive = green)
// saturation: 0 = grayscale, 1 = neutral, >1 = more saturation
// reinhard_white_point: ~1..4+ (higher = less compression)
struct post_params
{
    float2 screen_size;

    float bloom_strength;        // [0..1+] scales bloom contribution
    float exposure;              // EV (stops). 0=neutral, +1=2x, -1=0.5x
    int   tonemap_mode;          // 0=ACES, 1=Reinhard, 2=None
    float saturation;            // 0..2 (typical)

    float wb_temp;               // -1..+1 (blue<->orange)
    float wb_tint;               // -1..+1 (magenta<->green)
    float reinhard_white_point;  // ~3.0 default
    float _pad_;
};

SamplerState smp_linear : static_sampler_linear;

//------------------------------------------------------------------------------
// Fullscreen triangle
//------------------------------------------------------------------------------
vs_output VSMain(uint vertexID : SV_VertexID)
{
    vs_output OUT;
    float2 pos;
    if (vertexID == 0) pos = float2(-1.0, -1.0);
    else if (vertexID == 1) pos = float2(-1.0,  3.0);
    else                    pos = float2( 3.0, -1.0);
    OUT.pos = float4(pos, 0.0, 1.0);
    OUT.uv  = pos * float2(0.5, -0.5) + 0.5;
    return OUT;
}

//------------------------------------------------------------------------------
// Utility: luminance (Rec.709)
//------------------------------------------------------------------------------
float Luma709(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

//------------------------------------------------------------------------------
// White balance in LMS space (approx. von Kries adaptation)
//   wb_temp shifts red/blue; wb_tint shifts green/magenta.
//   Ranges -1..+1 are practical
//------------------------------------------------------------------------------
float3 white_balance(float3 color, float wb_temp, float wb_tint)
{
    // RGB -> LMS
    const float3x3 LIN_2_LMS = float3x3(
        0.390405f, 0.549941f, 0.0089263f,
        0.070841f, 0.963172f, 0.0013578f,
        0.023108f, 0.128021f, 0.936245f);

    const float3x3 LMS_2_LIN = float3x3(
        2.85847f,  -1.62879f, -0.024891f,
       -0.210182f,  1.15820f,  0.000324f,
        0.025096f, -0.118169f,  1.01504f);

    float3 lms = mul(LIN_2_LMS, max(color, 0.0));

    // Scale strengths: tune to taste
    float t = wb_temp * 0.1;   // temperature strength
    float g = wb_tint * 0.1;   // tint strength

    // Apply diagonal gains in LMS
    float3 gains = float3(1.0 + t - 0.5*g,  // L ~ red-ish
                          1.0 + g,          // M ~ green
                          1.0 - t - 0.5*g); // S ~ blue-ish
    lms = lms * gains;

    float3 outRGB = mul(LMS_2_LIN, lms);
    return max(outRGB, 0.0);
}

//------------------------------------------------------------------------------
// Saturation: lerp between grayscale and original
//------------------------------------------------------------------------------
float3 adjust_saturation(float3 color, float saturation)
{
    float y = Luma709(color);
    return lerp(y.xxx, color, saturation);
}

//------------------------------------------------------------------------------
// ACES fitted (RRT+ODT) in sRGB/Rec
//------------------------------------------------------------------------------
float3 rrt_odt_fit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 aces_fitted(float3 color)
{
    const float3x3 ACESInputMat = float3x3(
        0.59719f, 0.35458f, 0.04823f,
        0.07600f, 0.90834f, 0.01566f,
        0.02840f, 0.13383f, 0.83777f);

    const float3x3 ACESOutputMat = float3x3(
        1.60475f, -0.53108f, -0.07367f,
       -0.10208f,  1.10813f, -0.00605f,
       -0.00327f, -0.07276f,  1.07602f);

    color = mul(ACESInputMat, color);
    color = rrt_odt_fit(color);
    color = mul(ACESOutputMat, color);
    return saturate(color);
}

//------------------------------------------------------------------------------
// Reinhard (extended) per-channel with white point
//------------------------------------------------------------------------------
float3 reinhard_extended(float3 color, float white_point)
{
    // Per-channel extended Reinhard: c * (1 + c/Lw2) / (1 + c)
    // Using scalar white point per channel for simplicity.
    float3 wp2 = white_point * white_point;
    return (color * (1.0 + color / wp2)) / (1.0 + color);
}

#ifdef USE_SELECTION_OUTLINE


float4 outline(float2 uv, float2 texel_size, float thickness, Texture2D texture)
{
	float2 diffUV = texel_size.xy * float2(thickness, thickness);
 
	uv.x = uv.x - diffUV.x;
	uv.y = uv.y;
	float4 xDif = texture.SampleLevel(smp_linear, uv, 0);
	uv.x = uv.x + diffUV.x;
	xDif -= texture.SampleLevel(smp_linear, uv, 0);

	uv.x = uv.x;
	uv.y = uv.y - diffUV.y;
	float4 yDif = texture.SampleLevel(smp_linear, uv, 0);
	uv.y = uv.y + diffUV.y;
	yDif -= texture.SampleLevel(smp_linear, uv, 0);
	return sqrt(xDif*xDif + yDif*yDif);
}

#endif

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
    post_params params = sfg_get_cbv<post_params>(sfg_rp_constant0);
    Texture2D<float4> tex_lighting = sfg_get_texture<Texture2D<float4> >(sfg_rp_constant1);
    Texture2D<float4> tex_bloom    = sfg_get_texture<Texture2D<float4> >(sfg_rp_constant2);

#ifdef USE_SELECTION_OUTLINE
    Texture2D tex_selection   = sfg_get_texture<Texture2D>(sfg_rp_constant3);
    float2 texel_size = float2(1.0 / params.screen_size.x, 1.0 / params.screen_size.y);
    float4 selection_color = outline(IN.uv, texel_size, 2.0, tex_selection);
#endif

    // Fetch HDR inputs (mip 0). 
    float3 lighting = tex_lighting.SampleLevel(smp_linear, IN.uv, 0).rgb;
    float3 bloom    = tex_bloom.SampleLevel(smp_linear, IN.uv, 0).rgb;
    // Combine HDR
    float3 hdr = lighting + bloom * params.bloom_strength;

    // Exposure (EV to linear)
    hdr *= exp2(params.exposure);

    // guard against tiny negatives before grading/tonemap
    hdr = max(hdr, 0.0);

    // White balance (before saturation)
    hdr = white_balance(hdr, params.wb_temp, params.wb_tint);

    // Saturation in HDR (common practice)
    hdr = adjust_saturation(hdr, params.saturation);

    // Tonemap to LDR (still linear domain)
    int tonemap_mode = params.tonemap_mode;

    float3 ldr;
    if (tonemap_mode == 0)        // ACES
    {
        ldr = aces_fitted(hdr);
    }
    else if (tonemap_mode == 1)   // Reinhard
    {
        float wp = max(params.reinhard_white_point, 1e-3);
        ldr = reinhard_extended(hdr, wp);
        ldr = saturate(ldr);
    }
    else   // None / bypass (for debugging)
    {
        // Useful to inspect raw HDR; clamp so it stays representable.
        ldr = saturate(hdr);
    }

#ifdef USE_SELECTION_OUTLINE
    return float4(ldr, 1.0) + selection_color;
#endif
    return float4(ldr, 1.0);
}
