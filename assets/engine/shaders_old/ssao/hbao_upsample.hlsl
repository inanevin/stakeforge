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

// ----------------------------------------------
// Constants / Params
// ----------------------------------------------

// sfg_rp_constant0 - ubo
// sfg_rp_constant1 - R32 depth full res
// sfg_rp_constant2 - RGB10A2 World Normals full res
// sfg_rp_constant3 - R8 unorm half_res ao output from prev.
// sfg_rp_constant4 - R8 unorm full_res ao output.

struct ao_params
{
    float4x4 proj;        // camera projection matrix
    float4x4 inv_proj;    // inverse of proj
    float4x4 view_matrix; // 3x3 of camera view.

    // Full-resolution render target size (camera buffer)
    uint2 full_size;           // e.g., (W, H)
    uint2 half_size;           // e.g., (W/2, H/2)
    float2 inv_full;           // 1/full_size
    float2 inv_half;           // 1/half_size

    // Projection
    float z_near;               // near plane
    float z_far;                // far plane 

    // AO controls
    float radius_world;         // AO radius in world 
    float bias;                 // small bias to reduce self-occlusion (0.02)
    float intensity;            // AO strength (1.2)
    float power;                // contrast shaping (, 1.1)
    uint  num_dirs;             // 8
    uint  num_steps;            // 6
    float random_rot_strength;  // 1.0
};

// ----------------------------------------------
// Resources
// ----------------------------------------------

SamplerState smp_nearest  : static_sampler_nearest;
SamplerState smp_nearest_repeat  : static_sampler_nearest_repeat;
SamplerState smp_linear : static_sampler_linear;

// ----------------------------------------------
// Helpers
// ----------------------------------------------

// Map UV [0,1] (v down) -> NDC [-1,1] (y up)
float2 uv_to_ndc(float2 uv)
{
    float2 ndc;
    ndc.x = uv.x * 2.0f - 1.0f;
    ndc.y = (1.0f - uv.y) * 2.0f - 1.0f;   // <-- flip Y here
    return ndc;
}

// Map NDC [-1,1] (y up) -> UV [0,1] (v down)
float2 ndc_to_uv(float2 ndc)
{
    float2 uv;
    uv.x = 0.5f * ndc.x + 0.5f;
    uv.y = 0.5f * (-ndc.y) + 0.5f;     
    return uv;
}

float3 view_pos_from_uv_depth(float2 uv, float z01, float4x4 inv_proj)
{
    float2 ndc_xy = uv_to_ndc(uv);
    float4 clip   = float4(ndc_xy, z01, 1.0f);
    float4 view   = mul(inv_proj, clip);
    return view.xyz / view.w;   
}

float3 fetch_view_normal(float2 uv, float3x3 world_to_view)
{
    Texture2D norm_enc_full = sfg_get_texture<Texture2D>(sfg_rp_constant2);
    float2 enc = norm_enc_full.SampleLevel(smp_nearest, uv, 0).xy;
    float3 n_world = oct_decode(enc);
    float3 n_view  = mul(world_to_view, n_world);
    return normalize(n_view);               
}

// ----------------------------------------------
// Main CS
// ----------------------------------------------
[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    ao_params params = sfg_get_cbv<ao_params>(sfg_rp_constant0);

    uint2 pF = DTid.xy;
    if (pF.x >= params.full_size.x || pF.y >= params.full_size.y) return;

    float2 uvF = (pF + 0.5) * params.inv_full;

    // Guides at full-res
    RWTexture2D<unorm float> AO_full = sfg_get_texture<RWTexture2D<float> >(sfg_rp_constant4); 
    Texture2D<float> depth_full = sfg_get_texture<Texture2D<float> >(sfg_rp_constant1);
    float zF   = depth_full.SampleLevel(smp_nearest, uvF, 0);
    if (is_background(zF))
    {
        AO_full[pF] = 1.0;
        return;
    }
    float3 pVF = view_pos_from_uv_depth(uvF, zF, params.inv_proj);
    float3 nVF = fetch_view_normal(uvF, (float3x3)params.view_matrix);

    // Base half-res texel that covers this full pixel
    uint2 pH_base = pF >> 1;

    Texture2D<float> AO_half = sfg_get_texture<Texture2D<float> >(sfg_rp_constant3);

    float ao_num = 0.0, ao_den = 0.0;

    // 3x3 neighborhood in HALF space
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    [unroll] for (int dx = -1; dx <= 1; ++dx)
    {
        int2 qH = int2(pH_base) + int2(dx, dy);
        if (qH.x < 0 || qH.y < 0 || qH.x >= params.half_size.x || qH.y >= params.half_size.y) continue;

        // sample AO at HALF texel center
        float2 uvH = (float2(qH) + 0.5) * params.inv_half;
        float  aoH = AO_half.SampleLevel(smp_nearest, uvH, 0);

        // guidance at the corresponding FULL uv of this half texel
        float2 uvH_asFull = (float2(qH) * 2.0 + 1.0) * params.inv_full;
        float  zQ   = depth_full.SampleLevel(smp_nearest, uvH_asFull, 0);
        float3 pVQ  = view_pos_from_uv_depth(uvH_asFull, zQ, params.inv_proj);
        float3 nVQ  = fetch_view_normal(uvH_asFull, (float3x3)params.view_matrix);

        // weights (tune sigmas)
        float  w_sp = exp(-float(dx*dx + dy*dy) / (2.0 * 1.5 * 1.5));
        float  dz   = abs((-pVQ.z) - (-pVF.z));   // camera-depth diff
        float  w_z  = exp(-(dz*dz)/(2.0 * 1.0 * 1.0));
        float  w_n  = saturate(dot(nVF, nVQ));    // or pow(dot,4)
        float  w    = w_sp * w_z * pow(w_n, 4.0);

        ao_num += w * aoH;
        ao_den += w;
    }

    float ao_full = (ao_den > 0.0) ? ao_num / ao_den : AO_half.Load(int3(pH_base,0));

    AO_full[pF] = ao_full; 
}
