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

// =====================================================================================
// HBAO Half-Resolution Compute
// =====================================================================================

#ifndef PI
#define PI 3.14159265358979323846
#endif

// ----------------------------------------------
// Constants / Params
// ----------------------------------------------

// sfg_rp_constant0 - ubo
// sfg_rp_constant1 - R32 depth full res
// sfg_rp_constant2 - RGB10A2 World Normals full res
// sfg_rp_constant3 - noise tex
// sfg_rp_constant4 - R8 unorm half_res ao output.

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
    float radius_world;         // AO radius in *world/view* units (meters)
    float bias;                 // small bias to reduce self-occlusion (e.g., 0.02)
    float intensity;            // AO strength (e.g., 1.2)
    float power;                // contrast shaping (e.g., 1.1)
    uint  num_dirs;             // e.g., 8
    uint  num_steps;            // e.g., 6
    float random_rot_strength;  // e.g., 1.0
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

float2 project_to_uv(float3 vView, float4x4 proj)
{
    float4 clip = mul(proj, float4(vView, 1.0f));
    float2 ndc  = clip.xy / clip.w;
    return ndc_to_uv(ndc);              
}

float3 fetch_view_pos(float2 uv, float4x4 inv_proj)
{
    Texture2D<float> depth_full = sfg_get_texture<Texture2D<float> >(sfg_rp_constant1);
    float z01 = depth_full.SampleLevel(smp_nearest, uv, 0);
    return view_pos_from_uv_depth(uv, z01, inv_proj); 
}

float3 fetch_view_normal(float2 uv, float3x3 world_to_view)
{
    Texture2D norm_enc_full = sfg_get_texture<Texture2D>(sfg_rp_constant2);
    float2 enc = norm_enc_full.SampleLevel(smp_nearest, uv, 0).xy;
    float3 n_world = oct_decode(enc);
    float3 n_view  = mul(world_to_view, n_world);
    return normalize(n_view);               
}

// Build an orthonormal basis (T,B) around normal N
void build_tangent_frame(float3 N, out float3 T, out float3 B)
{
    float3 up = (abs(N.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// Distance falloff (tweakable). Keeps nearby occluders stronger.
float falloff(float dist, float rad)
{
    // Inverse quad with small constant to avoid singularity
   float ratio = saturate(dist / rad);
   return saturate(1.0 - ratio * ratio); // 1.0 at dist=0, 0.0 at dist=rad
}

// Signed camera-depth (more robust than just z)
float depth_cam(float3 v) { return -dot(v, float3(0,0,1)); } // larger = further

// ----------------------------------------------
// Main CS
// ----------------------------------------------
[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // Half-res pixel
    uint2 pH = DTid.xy;
    ao_params params = sfg_get_cbv<ao_params>(sfg_rp_constant0);

    if (pH.x >= params.half_size.x || pH.y >= params.half_size.y)
        return;

    // Center
    float2 uv_full = ((float2(pH) * 2.0f + 1.0f) * params.inv_full);

    //uint2 p = DTid.xy;
    //if (p.x >= params.full_size.x || p.y >= params.full_size.y) return;
    // Center
    //float2 uv_full = (float2(p) + 0.5) * params.inv_full;         // direct full-res uv
    RWTexture2D<unorm float> ao_half_out = sfg_get_texture<RWTexture2D<float> >(sfg_rp_constant4);
    Texture2D<float> depth_full = sfg_get_texture<Texture2D<float> >(sfg_rp_constant1);
    float z01 = depth_full.SampleLevel(smp_nearest, uv_full, 0);
    if (is_background(z01))
    {
        ao_half_out[pH] = 1.0;
        return;
    }

    // View-space center position & normal<
    float3 pC = fetch_view_pos(uv_full, params.inv_proj);          // view-space position
    float3 nC = fetch_view_normal(uv_full, (float3x3)params.view_matrix);

    // Tangent frame
    float3 T,B; build_tangent_frame(nC, T, B);

    // Per-pixel random rotation (prevents banding)
    // Tile the small noise over full-res UV
    Texture2D noise_tex = sfg_get_texture<Texture2D>(sfg_rp_constant3);
    float2 noiseUV = uv_full * (params.full_size / 8.0); // tile 8x8 across full-res
    float2 noise = noise_tex.SampleLevel(smp_nearest_repeat, noiseUV, 0).xy;

    float rot = atan2(noise.y, noise.x) * params.random_rot_strength ;

    // Accumulate occlusion over directions and steps
    float ao_accum = 0.0f;

    uint dirs = params.num_dirs ;
    uint steps = params.num_steps ;
    float rad = params.radius_world ;
    float intensity = params.intensity  ;
    float power = params.power ;
    
    // March
    for (uint d = 0; d < dirs; ++d)
    {
        float ang = rot + (2.0 * PI) * (d / (float)dirs);
        float3 omega = normalize(T * cos(ang) + B * sin(ang));

        // Track the horizon (maximum elevation angle) for this direction
        float alpha_max = -PI * 0.5f;   // start below the plane

        float max_horizon = 0.0;
        float jitter01 = frac(dot(noise, float2(12.9898, 78.233)) + d * 0.61803398875);

        for (uint s = 1; s < steps; ++s)
        {
            float stepLen = 1.0f / steps;
            // center-of-bin (0.5) + jitter shift
            float t = (s + 0.5 + jitter01) * stepLen;   // nominally (0,1+j/steps]
            if (t >= 1.0f) break;                       // optional clamp/early-exit

            float dist = t * rad;
            float3 qVS  = pC + omega * dist;

            // Project to fetch the ground-truth depth sample
            float2 uvQ = project_to_uv(qVS, params.proj);
            if (uvQ.x <= 0.0 || uvQ.x >= 1.0 || uvQ.y <= 0.0 || uvQ.y >= 1.0)
                break;

            float3 pS = fetch_view_pos(uvQ, params.inv_proj);

            float3 v  = pS - pC;

            // Signed components in the {omega, nC} slice
            float r = dot(v, omega);
            if (r <= 0.0f)      // only things ahead along omega can occlude
                continue;

            float h = dot(v, nC);

            // Bias to reduce self-occlusion
            h -= params.bias;

            // Elevation angle of this sample
            float alpha = atan2(h, r);

            // Weight by distance falloff 
            float w = falloff(dist, rad);

            // Raise the effective horizon
            alpha_max = max(alpha_max, alpha * w);
            
        }
 
        // Directional occlusion contribution: sin of horizon elevation
        // Clamp to [0, 1] — negative horizons mean no occluder above the plane.
        float dir_occl = saturate(sin(alpha_max));

        ao_accum += dir_occl;
    }

    // Normalize and shape
    float maxAccum = (float)params.num_dirs * (float)params.num_steps;    // loose upper bound
    float ao = ao_accum / (0.5f * maxAccum);               // heuristic normalization
    ao = saturate(ao * intensity);
    ao = pow(1.0 - ao, power);

    // Write half-res AO
    ao_half_out[pH] = ao;
}
