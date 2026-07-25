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

#include "pbr.hlsl"

static const uint SFG_INVALID_GPU_INDEX = 0xffffffffu;

struct gpu_light
{
	float4 position_range;
	float4 direction_param0;
	float4 right_param1;
	float4 color_intensity;
	uint4 shadow;
};

struct gpu_shadow_view
{
	float4x4 view_proj;
	float4 params0;
	float4 params1;
	float4 params2;
	uint texture_index;
	uint slice;
	uint type;
	uint pad;
};

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

float get_area_light_attenuation(gpu_light light, float3 world_pos, out float3 L)
{
	const float3 direction = normalize(light.direction_param0.xyz);
	const float3 right = normalize(light.right_param1.xyz);
	const float3 up = normalize(cross(direction, right));
	const float half_height = abs(light.direction_param0.w);
	const float half_width = abs(light.right_param1.w);
	const float3 delta = world_pos - light.position_range.xyz;
	const float x = clamp(dot(delta, right), -half_width, half_width);
	const float y = clamp(dot(delta, up), -half_height, half_height);
	const float3 closest = light.position_range.xyz + right * x + up * y;
	const float3 light_vector = closest - world_pos;
	const float distance_to_light = max(length(light_vector), 1e-4);
	L = light_vector / distance_to_light;
	float facing = dot(-L, direction);
	if (light.direction_param0.w < 0.0)
		facing = abs(facing);
	else
		facing = saturate(facing);
	const float area = 4.0 * half_width * half_height;
	return get_range_attenuation(light.position_range.w, distance_to_light) * area * facing / max(distance_to_light * distance_to_light, area);
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
    float2         shadow_resolution, float texel_world, float depth_bias, float normal_bias)
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

    float NoL = saturate(dot(N, normalize(L)));
    float compare_depth = ndc.z - depth_bias - normal_bias * (1.0 - NoL);

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
    float cos_outer, float depth_bias, float normal_bias
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
    float receiver_bias = depth_bias + normal_bias * (1.0 - NoL);

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
    float near_z, float far_z, float depth_bias, float normal_bias
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
    float  receiver_bias = depth_bias + normal_bias * (1.0 - NoL);

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
    float          blend_width, float texel_world, float depth_bias, float normal_bias)
{
    if (blend_width <= 0.0f || slice_next < 0)
    {
        return sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world, depth_bias, normal_bias);
    }

    float a = saturate( (split_curr - depth_linear) / max(blend_width, 1e-6f) );
    // a=1 => fully current cascade, a=0 => transition to next
    float s0 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world, depth_bias, normal_bias);
    float s1 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_next, world_pos, N, L, slice_next, shadow_resolution, texel_world, depth_bias, normal_bias);
    
    return lerp(s1, s0, a);
}

struct surface_lighting_data
{
	float3 world_pos;
	float3 view_direction;
	float3 normal;
	float3 albedo;
	float ambient_occlusion;
	float roughness;
	float metallic;
};

struct scene_lighting_data
{
	float4x4 view;
	uint4 light_counts;
};

float evaluate_directional_shadow(
	gpu_light light,
	surface_lighting_data surface,
	float4x4 view,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler,
	float3 light_direction)
{
	if (light.shadow.x == SFG_INVALID_GPU_INDEX)
		return 1.0;

	const float depth_linear = abs(mul(view, float4(surface.world_pos, 1.0)).z);
	uint cascade = 0;

	// grab the cascade covering this pixel
	[loop]
	for (uint i = 0; i + 1 < light.shadow.y && depth_linear > shadow_buffer[light.shadow.x + i].params0.y; ++i)
		cascade = i + 1;

	const gpu_shadow_view shadow_view = shadow_buffer[light.shadow.x + cascade];
	const gpu_shadow_view last_shadow_view = shadow_buffer[light.shadow.x + light.shadow.y - 1];
	Texture2DArray shadow_map = sfg_get_texture<Texture2DArray>(shadow_view.texture_index);

	float shadow = sample_cascade_shadow(
		shadow_map,
		shadow_sampler,
		shadow_view.view_proj,
		surface.world_pos,
		surface.normal,
		light_direction,
		shadow_view.slice,
		1.0 / shadow_view.params1.xy,
		0.0,
		shadow_view.params2.x,
		shadow_view.params2.y);

	// ease across cascade seams
	if (cascade + 1 < light.shadow.y)
	{
		const float blend_width = max((shadow_view.params0.y - shadow_view.params0.x) * 0.1, 0.001);
		const float blend = saturate((depth_linear - (shadow_view.params0.y - blend_width)) / blend_width);

		if (blend > 0.0)
		{
			const gpu_shadow_view next_shadow_view = shadow_buffer[light.shadow.x + cascade + 1];
			const float next_shadow = sample_cascade_shadow(
				shadow_map,
				shadow_sampler,
				next_shadow_view.view_proj,
				surface.world_pos,
				surface.normal,
				light_direction,
				next_shadow_view.slice,
				1.0 / next_shadow_view.params1.xy,
				0.0,
				next_shadow_view.params2.x,
				next_shadow_view.params2.y);

			shadow = lerp(shadow, next_shadow, blend);
		}
	}

	// fade out before the cascade coverage ends
	const float distance_fade = saturate((last_shadow_view.params0.y - depth_linear) / max(last_shadow_view.params2.w, 0.001));

	return lerp(1.0, shadow, shadow_view.params2.z * distance_fade);
}

float evaluate_point_shadow(
	gpu_light light,
	surface_lighting_data surface,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler,
	float3 light_direction)
{
	if (light.shadow.x == SFG_INVALID_GPU_INDEX)
		return 1.0;

	const gpu_shadow_view shadow_view = shadow_buffer[light.shadow.x];
	TextureCube shadow_map = sfg_get_texture<TextureCube>(shadow_view.texture_index);
	const float shadow = sample_shadow_cube(
		shadow_map,
		shadow_sampler,
		shadow_view.view_proj,
		light.position_range.xyz,
		surface.world_pos,
		surface.normal,
		light_direction,
		1.0 / shadow_view.params1.xy,
		shadow_view.params0.z,
		shadow_view.params0.w,
		shadow_view.params2.x,
		shadow_view.params2.y);

	return lerp(1.0, shadow, shadow_view.params2.z);
}

float evaluate_spot_shadow(
	gpu_light light,
	surface_lighting_data surface,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler,
	float3 light_direction,
	float3 spot_direction,
	float cos_outer)
{
	if (light.shadow.x == SFG_INVALID_GPU_INDEX)
		return 1.0;

	const gpu_shadow_view shadow_view = shadow_buffer[light.shadow.x];
	Texture2D shadow_map = sfg_get_texture<Texture2D>(shadow_view.texture_index);
	const float shadow = sample_shadow_cone(
		shadow_map,
		shadow_sampler,
		shadow_view.view_proj,
		surface.world_pos,
		surface.normal,
		light_direction,
		spot_direction,
		1.0 / shadow_view.params1.xy,
		cos_outer,
		shadow_view.params2.x,
		shadow_view.params2.y);

	return lerp(1.0, shadow, shadow_view.params2.z);
}

float3 evaluate_directional_light(
	gpu_light light,
	surface_lighting_data surface,
	float4x4 view,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler)
{
	const float3 light_direction = normalize(-light.direction_param0.xyz);
	const float shadow = evaluate_directional_shadow(light, surface, view, shadow_buffer, shadow_sampler, light_direction);
	const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * shadow);

	return calculate_pbr(
		surface.view_direction,
		surface.normal,
		light_direction,
		surface.albedo,
		surface.ambient_occlusion,
		surface.roughness,
		surface.metallic,
		radiance);
}

float3 evaluate_point_light(
	gpu_light light,
	surface_lighting_data surface,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler)
{
	const float3 light_vector = light.position_range.xyz - surface.world_pos;
	const float distance_to_light = max(length(light_vector), 1e-4);
	const float3 light_direction = light_vector / distance_to_light;
	const float light_attenuation = attenuation(light.position_range.w, distance_to_light);
	const float shadow = evaluate_point_shadow(light, surface, shadow_buffer, shadow_sampler, light_direction);
	const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation * shadow);

	return calculate_pbr(
		surface.view_direction,
		surface.normal,
		light_direction,
		surface.albedo,
		surface.ambient_occlusion,
		surface.roughness,
		surface.metallic,
		radiance);
}

float3 evaluate_spot_light(
	gpu_light light,
	surface_lighting_data surface,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler)
{
	const float3 light_vector = light.position_range.xyz - surface.world_pos;
	const float distance_to_light = max(length(light_vector), 1e-4);
	const float3 light_direction = light_vector / distance_to_light;
	const float3 spot_direction = normalize(light.direction_param0.xyz);
	const float cos_inner = max(light.direction_param0.w, light.right_param1.w);
	const float cos_outer = min(light.direction_param0.w, light.right_param1.w);
	const float cone = smoothstep(cos_outer, cos_inner, dot(-light_direction, spot_direction));
	const float light_attenuation = attenuation(light.position_range.w, distance_to_light) * cone;
	const float shadow = evaluate_spot_shadow(light, surface, shadow_buffer, shadow_sampler, light_direction, spot_direction, cos_outer);
	const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation * shadow);

	return calculate_pbr(
		surface.view_direction,
		surface.normal,
		light_direction,
		surface.albedo,
		surface.ambient_occlusion,
		surface.roughness,
		surface.metallic,
		radiance);
}

float3 evaluate_area_light(gpu_light light, surface_lighting_data surface)
{
	float3 light_direction = 0.0.xxx;
	const float light_attenuation = get_area_light_attenuation(light, surface.world_pos, light_direction);
	const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation);

	return calculate_pbr(
		surface.view_direction,
		surface.normal,
		light_direction,
		surface.albedo,
		surface.ambient_occlusion,
		surface.roughness,
		surface.metallic,
		radiance);
}

float3 evaluate_scene_lighting(
	surface_lighting_data surface,
	scene_lighting_data scene,
	StructuredBuffer<gpu_light> light_buffer,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	SamplerComparisonState shadow_sampler,
	SamplerComparisonState shadow_cube_sampler)
{
	float3 lighting = 0.0.xxx;
	uint light_offset = 0;

	[loop]
	for (uint i = 0; i < scene.light_counts.x; ++i)
		lighting += evaluate_directional_light(light_buffer[light_offset + i], surface, scene.view, shadow_buffer, shadow_sampler);

	light_offset += scene.light_counts.x;

	[loop]
	for (uint i = 0; i < scene.light_counts.y; ++i)
		lighting += evaluate_point_light(light_buffer[light_offset + i], surface, shadow_buffer, shadow_cube_sampler);

	light_offset += scene.light_counts.y;

	[loop]
	for (uint i = 0; i < scene.light_counts.z; ++i)
		lighting += evaluate_spot_light(light_buffer[light_offset + i], surface, shadow_buffer, shadow_sampler);

	light_offset += scene.light_counts.z;

	[loop]
	for (uint i = 0; i < scene.light_counts.w; ++i)
		lighting += evaluate_area_light(light_buffer[light_offset + i], surface);

	return lighting;
}

float3 evaluate_clustered_scene_lighting(
	surface_lighting_data surface,
	scene_lighting_data scene,
	StructuredBuffer<gpu_light> light_buffer,
	StructuredBuffer<gpu_shadow_view> shadow_buffer,
	StructuredBuffer<uint> cluster_light_indices,
	uint cluster_light_offset,
	uint cluster_light_count,
	SamplerComparisonState shadow_sampler,
	SamplerComparisonState shadow_cube_sampler)
{
	float3 lighting = 0.0.xxx;

	// directional lights still touch the whole view
	[loop]
	for (uint i = 0; i < scene.light_counts.x; ++i)
		lighting += evaluate_directional_light(light_buffer[i], surface, scene.view, shadow_buffer, shadow_sampler);

	const uint point_light_end = scene.light_counts.x + scene.light_counts.y;
	const uint spot_light_end = point_light_end + scene.light_counts.z;

	// local lights come straight from this pixel's cluster
	[loop]
	for (uint i = 0; i < cluster_light_count; ++i)
	{
		const uint light_index = cluster_light_indices[cluster_light_offset + i];
		const gpu_light light = light_buffer[light_index];

		if (light_index < point_light_end)
			lighting += evaluate_point_light(light, surface, shadow_buffer, shadow_cube_sampler);
		else if (light_index < spot_light_end)
			lighting += evaluate_spot_light(light, surface, shadow_buffer, shadow_sampler);
		else
			lighting += evaluate_area_light(light, surface);
	}

	return lighting;
}
