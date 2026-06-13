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
#include "normal.hlsl"

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct render_pass_data
{
	float4x4 inv_view_proj;
};

SamplerState smp_nearest : static_sampler_nearest;

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
	Texture2D tex_ao				 = sfg_get_texture<Texture2D>(sfg_constant_rp6);

	const int2	 pixel		  = int2(input.pos.xy);
	const float	 device_depth = tex_gbuffer_depth.Load(int3(pixel, 0)).r;
	if (is_background(device_depth))
		return float4(0.0, 0.0, 0.0, 1.0);

	const float3 world_pos = reconstruct_world_position(input.uv, device_depth, rp_data.inv_view_proj);
	const float3 albedo   = tex_gbuffer_color.SampleLevel(smp_nearest, input.uv, 0).xyz;
	const float4 normal   = tex_gbuffer_normal.Load(int3(pixel, 0));
	const float4 orm	   = tex_gbuffer_orm.Load(int3(pixel, 0));
	const float3 emissive = tex_gbuffer_emissive.Load(int3(pixel, 0)).xyz;
	const float	 ao	   = saturate(orm.r) * tex_ao.Load(int3(pixel, 0)).r;
	const float	 roughness = saturate(orm.g);
	const float	 metallic  = saturate(orm.b);
	const float3 N		   = oct_decode(normal.xy);

	return float4(albedo * ao + emissive, 1.0);
}
