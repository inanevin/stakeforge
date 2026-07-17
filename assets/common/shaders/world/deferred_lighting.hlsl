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
#include "depth.hlsl"
#include "light.hlsl"
#include "normal.hlsl"
#include "pbr.hlsl"

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct render_pass_data
{
	float4x4 inv_view_proj;
	float4x4 inv_view;
	float4 camera_pos;
	float4 skybox_params;
	uint4 light_counts;
};

SamplerState smp_nearest : static_sampler_nearest;
SamplerState smp_linear : static_sampler_linear;

static const uint SFG_INVALID_GPU_INDEX = 0xffffffffu;
static const float SFG_SKYBOX_PREFILTER_MAX_LOD = 7.0;

float3 get_sky_color(uint skybox_radiance, float2 uv, render_pass_data rp_data)
{
	if (skybox_radiance == SFG_INVALID_GPU_INDEX)
		return float3(0.0, 0.0, 0.0);

	TextureCube tex_skybox_radiance = sfg_get_texture<TextureCube>(skybox_radiance);
	const float3 far_world			 = reconstruct_world_position(uv, 0.0, rp_data.inv_view_proj);
	const float3 ray_dir			 = normalize(far_world - rp_data.camera_pos.xyz);
	return tex_skybox_radiance.SampleLevel(smp_linear, ray_dir, 0).rgb * rp_data.skybox_params.x * rp_data.skybox_params.y;
}

float3 fresnel_schlick_roughness(float cos_theta, float3 f0, float roughness)
{
	const float3 f90 = max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), f0);
	return f0 + (f90 - f0) * pow(1.0 - saturate(cos_theta), 5.0);
}

float3 get_sky_lighting(uint skybox_irradiance, uint skybox_prefilter, uint skybox_brdf_lut, float3 N, float3 V, float3 albedo, float ao, float roughness, float metallic, render_pass_data rp_data)
{
	if (skybox_irradiance == SFG_INVALID_GPU_INDEX || skybox_prefilter == SFG_INVALID_GPU_INDEX || skybox_brdf_lut == SFG_INVALID_GPU_INDEX)
		return albedo * ao;

	TextureCube tex_skybox_irradiance = sfg_get_texture<TextureCube>(skybox_irradiance);
	TextureCube tex_skybox_prefilter	 = sfg_get_texture<TextureCube>(skybox_prefilter);
	Texture2D	tex_skybox_brdf_lut	 = sfg_get_texture<Texture2D>(skybox_brdf_lut);

	const float3 F0			 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
	const float	 NdotV		 = saturate(dot(N, V));
	const float3 F			 = fresnel_schlick_roughness(NdotV, F0, roughness);
	const float3 kD			 = (1.0 - F) * (1.0 - metallic);
	const float3 irradiance	 = tex_skybox_irradiance.SampleLevel(smp_linear, N, 0).rgb;
	const float3 diffuse	 = irradiance * albedo;
	const float3 R			 = reflect(-V, N);
	const float3 prefiltered = tex_skybox_prefilter.SampleLevel(smp_linear, R, roughness * SFG_SKYBOX_PREFILTER_MAX_LOD).rgb;
	const float2 brdf		 = tex_skybox_brdf_lut.SampleLevel(smp_linear, float2(NdotV, roughness), 0).rg;
	const float3 specular	 = prefiltered * (F * brdf.x + brdf.y);

	return (kD * diffuse * ao + specular) * rp_data.skybox_params.x * rp_data.skybox_params.y;
}

vs_output VSMain(uint vertex_id : SV_VertexID)
{
	vs_output output;

	float2 pos;
	if (vertex_id == 0)
		pos = float2(-1.0, -1.0);
	else if (vertex_id == 1)
		pos = float2(-1.0, 3.0);
	else
		pos = float2(3.0, -1.0);

	output.pos = float4(pos, 0.0, 1.0);
	output.uv	= pos * float2(0.5, -0.5) + 0.5;
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_constant_rp0);

	Texture2D tex_gbuffer_color	 = sfg_get_texture<Texture2D>(sfg_constant_rp1);
	Texture2D tex_gbuffer_normal	 = sfg_get_texture<Texture2D>(sfg_constant_rp2);
	Texture2D tex_gbuffer_orm	 = sfg_get_texture<Texture2D>(sfg_constant_rp3);
	Texture2D tex_gbuffer_emissive = sfg_get_texture<Texture2D>(sfg_constant_rp4);
	Texture2D tex_gbuffer_depth	 = sfg_get_texture<Texture2D>(sfg_constant_rp5);
	Texture2D tex_ao					 = sfg_get_texture<Texture2D>(sfg_constant_rp6);
	const uint skybox_radiance		 = sfg_constant_rp7;
	const uint skybox_irradiance	 = sfg_constant_rp8;
	const uint skybox_prefilter		 = sfg_constant_rp9;
	const uint skybox_brdf_lut		 = sfg_constant_rp10;
	StructuredBuffer<gpu_light> light_buffer = sfg_get_ssbo<gpu_light>(sfg_constant_rp11);

	const int2	 pixel		  = int2(input.pos.xy);
	const float	 device_depth = tex_gbuffer_depth.Load(int3(pixel, 0)).r;
	if (is_background(device_depth))
		return float4(get_sky_color(skybox_radiance, input.uv, rp_data), 1.0);

	const float4 orm	   = tex_gbuffer_orm.Load(int3(pixel, 0));
	const float3 emissive = tex_gbuffer_emissive.Load(int3(pixel, 0)).xyz;
	if (orm.a >= 0.5)
		return float4(emissive, 1.0);

	const float3 world_pos = reconstruct_world_position(input.uv, device_depth, rp_data.inv_view_proj);
	const float3 V		   = normalize(rp_data.camera_pos.xyz - world_pos);
	const float3 albedo   = tex_gbuffer_color.Load(int3(pixel, 0)).xyz;
	const float4 normal   = tex_gbuffer_normal.Load(int3(pixel, 0));
	const float	 ao	   = saturate(orm.r) * tex_ao.SampleLevel(smp_nearest, input.uv, 0).r;
	const float	 roughness = saturate(orm.g);
	const float	 metallic  = saturate(orm.b);
	const float3 N		   = oct_decode(normal.xy);
	float3 lighting = get_sky_lighting(skybox_irradiance, skybox_prefilter, skybox_brdf_lut, N, V, albedo, ao, roughness, metallic, rp_data);
	uint light_offset = 0;

	[loop]
	for (uint i = 0; i < rp_data.light_counts.x; ++i)
	{
		const gpu_light light = light_buffer[light_offset + i];
		const float3 L = normalize(-light.direction_param0.xyz);
		const float3 radiance = light.color_intensity.xyz * light.color_intensity.w;
		lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
	}
	light_offset += rp_data.light_counts.x;

	[loop]
	for (uint i = 0; i < rp_data.light_counts.y; ++i)
	{
		const gpu_light light = light_buffer[light_offset + i];
		const float3 light_vector = light.position_range.xyz - world_pos;
		const float distance_to_light = max(length(light_vector), 1e-4);
		const float3 L = light_vector / distance_to_light;
		const float light_attenuation = attenuation(light.position_range.w, distance_to_light);
		const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation);
		lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
	}
	light_offset += rp_data.light_counts.y;

	[loop]
	for (uint i = 0; i < rp_data.light_counts.z; ++i)
	{
		const gpu_light light = light_buffer[light_offset + i];
		const float3 light_vector = light.position_range.xyz - world_pos;
		const float distance_to_light = max(length(light_vector), 1e-4);
		const float3 L = light_vector / distance_to_light;
		const float3 direction = normalize(light.direction_param0.xyz);
		const float cos_inner = max(light.direction_param0.w, light.right_param1.w);
		const float cos_outer = min(light.direction_param0.w, light.right_param1.w);
		const float cone = smoothstep(cos_outer, cos_inner, dot(-L, direction));
		const float light_attenuation = attenuation(light.position_range.w, distance_to_light) * cone;
		const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation);
		lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
	}
	light_offset += rp_data.light_counts.z;

	[loop]
	for (uint i = 0; i < rp_data.light_counts.w; ++i)
	{
		const gpu_light light = light_buffer[light_offset + i];
		float3 L;
		const float light_attenuation = get_area_light_attenuation(light, world_pos, L);
		const float3 radiance = light.color_intensity.xyz * (light.color_intensity.w * light_attenuation);
		lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
	}

	return float4(lighting + emissive, 1.0);
}
