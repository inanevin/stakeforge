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

struct gpu_point_light
{
    float4 color_entity_index;
    float4 intensity_range;
    float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
    float near_plane;
    float far_plane;
};

struct gpu_spot_light
{
   float4 color_entity_index;
   float4 intensity_range_inner_outer;
   float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
};

struct gpu_dir_light
{
    float4 color_entity_index;
    float4 intensity;
    float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
};

struct gpu_shadow_data
{
    float4x4 light_space_matrix;
    float texel_world;
};

// d = distance to light
float get_range_attenuation(float r, float d)
{
    if (r <= 0.0) return 1.0;
    float x = saturate(1.0 - (d*d) / (r*r)); // 1 - (d/r)^2   in [0,1]
    return x * x;                             // smooth rolloff
}

float attenuation(float r, float d)
{
    return get_range_attenuation(r, d) / max(d*d, 1e-4); // inverse-square * clamp
}

static const float g_default_spot_blend = 0.2; 
static const float g_softness_exp       = 1.0;    // >1 = sharper, <1 = softer

float compute_cosInner(float cosInnerPacked, float cosOuter, float blend)
{
    if (cosInnerPacked > cosOuter + 1e-5) return cosInnerPacked;

    float thetaO = acos(saturate(cosOuter));
    float thetaI = thetaO * (1.0 - saturate(blend));
    return cos(thetaI);
}

float spot_blend_hermite(float cosTheta, float cosOuter, float cosInner, float softnessExp)
{
    float denom = max(cosInner - cosOuter, 1e-4);
    float x = saturate((cosTheta - cosOuter) / denom);
    // hermite
    x = x * x * (3.0 - 2.0 * x);
    // optional shaping
    return pow(x, max(softnessExp, 1e-3));
}


float2 ndc_to_uv(float2 ndc_xy) { return ndc_xy * 0.5f + 0.5f; }

static const float g_depth_bias_base   = 0.0001f;   // base receiver bias (world->light clip->depth)
static const float g_normal_bias_scale = 0.01f;     // scales with slope (bigger -> fewer acne, more peter-panning)
static const int   g_pcf_radius        = 1;         // 0: 1 tap, 1: 3x3, 2: 5x5
static const float g_cascade_blend     = 0.5f;      // optional cross-fade width in normalized depth (0 = off)

// Normal/slope-based bias term (NoL-aware).
float slope_bias(float NoL)
{
    float s = sqrt(saturate(1.0f - NoL * NoL));
    return g_normal_bias_scale * (s / max(NoL, 1e-3f));
}

bool outside(float2 uv) { return any(uv < 0.0f) || any(uv > 1.0f); }

float pcf_cascade(int radius, Texture2DArray shadow_map, SamplerComparisonState smp, float2 uv, int slice, float compare_depth, float2 texel)
{
     // PCF kernel (square)
    int r = radius;                 // 1 => 3x3, 2 => 5x5
    float taps = 0.0f;
    float accum = 0.0f;

    [unroll]
    for (int dy = -r; dy <= +r; ++dy)
    {
        [unroll]
        for (int dx = -r; dx <= +r; ++dx)
        {
            float2 offs = float2(dx, dy) * texel;
            accum += shadow_map.SampleCmpLevelZero(smp, float3(uv + offs, slice), compare_depth);
            taps += 1.0f;
        }
    }
    return accum / max(taps, 1.0f);
}

float pcf(int radius, Texture2D shadow_map, SamplerComparisonState smp, float2 uv, float compare_depth, float2 texel)
{
     // PCF kernel (square)
    int r = radius;                 // 1 => 3x3, 2 => 5x5
    float taps = 0.0f;
    float accum = 0.0f;

    [unroll]
    for (int dy = -r; dy <= +r; ++dy)
    {
        [unroll]
        for (int dx = -r; dx <= +r; ++dx)
        {
            float2 offs = float2(dx, dy) * texel;
            accum += shadow_map.SampleCmpLevelZero(smp, float2(uv + offs), compare_depth);
            taps += 1.0f;
        }
    }
    return accum / max(taps, 1.0f);
}

float sample_cascade_shadow(
    Texture2DArray shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3         world_pos,
    float3         N,
    float3         L,
    int            slice,
    float2         shadow_resolution, float texel_world)
{

    // Transform world position into light clip space
    float4 clip = mul(light_space_matrix, float4(world_pos, 1.0f));
    if (clip.w <= 0.0f) return 1.0f;
    float3 ndc = clip.xyz / clip.w;
    float2 uv = ndc_to_uv(ndc.xy);
    if (outside(uv)) return 1.0f;
    uv.y = 1.0 - uv.y;

    float2 texel = 1.0f / shadow_resolution;

    // we try to trust rasterizer bias, if not below
    // float NoL = saturate(dot(N, normalize(L)));
    // float receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y); // or texel_world * 0.00001
    // float compare_depth = ndc.z - receiver_bias * 0.1;

    float compare_depth = clip.z; 

     // Single tap (fast path)
    if (g_pcf_radius == 0)
    {
        float val = shadow_map.SampleCmpLevelZero(smp, float3(uv, slice), compare_depth);
        return val;
    }

    return pcf_cascade(g_pcf_radius, shadow_map, smp, uv, slice, compare_depth, texel);
}

float sample_shadow_cone(Texture2D shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3         world_pos,
    float3         N,
    float3         L,
    float3 light_forward,
    float2         shadow_resolution,
    float cos_outer
    )
{
    float cos_theta = dot(normalize(-L), normalize(light_forward));
    if (cos_theta < cos_outer) return 1.0f;

    float4 clip = mul(light_space_matrix, float4(world_pos, 1.0f));

    // Guard against points behind the light camera
    if (clip.w <= 0.0f) return 1.0f;

    float3 ndc = clip.xyz;
    ndc /= clip.w;

    float2 uv = ndc_to_uv(ndc.xy);
    if (outside(uv)) return 1.0f;
    uv.y = 1.0 - uv.y;

    float NoL = saturate(dot(N, normalize(L)));
    float2 texel = 1.0f / shadow_resolution;
    float receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y);

    float compare_depth = ndc.z - receiver_bias * 0.1;

    if (g_pcf_radius == 0)
    {
        float val = shadow_map.SampleCmpLevelZero(smp, uv, compare_depth);
        return val;
    }

    return pcf(g_pcf_radius, shadow_map, smp, uv,  compare_depth, texel);
}

float depth01_from_eyeZ(float z_eye, float nearZ, float farZ)
{
    float a =  farZ / (farZ - nearZ);
    float b = -nearZ * farZ / (farZ - nearZ);
    return a + b / z_eye; 
}

float sample_shadow_cube(TextureCube shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3 light_pos,
    float3         world_pos,
    float3         N,
    float3         L,
    float2         shadow_resolution,
    float near_z, float far_z
    )
{

    float3 R   = world_pos - light_pos;
    float3 dir = normalize(R);
    dir.z = -dir.z;
    
    // Eye-space z for the face the cube lookup will choose
    float z_eye = max(abs(R.x), max(abs(R.y), abs(R.z)));

    // Depth in 0..1 matching what was written to the cube depth faces
    float depth01 = depth01_from_eyeZ(z_eye, near_z, far_z);

    float2 texel = 1.0 / shadow_resolution;
    float  NoL   = saturate(dot(N, normalize(L)));
    float  receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y);

    float  cmp = depth01 - receiver_bias * 0.1;
    return shadow_map.SampleCmpLevelZero(smp, dir, cmp);
}

float sample_cascade_shadow_blend(
    Texture2DArray shadow_map,
    SamplerComparisonState smp_shadow,
    float4x4       light_space_matrix_curr,
    float4x4       light_space_matrix_next,
    float3         world_pos,
    float3         N,
    float3         L,
    int            slice_curr,
    int            slice_next,
    float2         shadow_resolution,
    float          depth_linear,           // eye-linear 
    float          split_curr,             // end depth of current cascade
    float          blend_width, float texel_world)    
{
    if (blend_width <= 0.0f || slice_next < 0)
    {
        return sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world);
    }

    float a = saturate( (split_curr - depth_linear) / max(blend_width, 1e-6f) );
    // a=1 => fully current cascade, a=0 => transition to next
    float s0 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world);
    float s1 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_next, world_pos, N, L, slice_next, shadow_resolution, texel_world);
    
    return lerp(s1, s0, a);
}