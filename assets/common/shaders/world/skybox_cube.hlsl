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
#include "render_pass_defines.hlsl"

SFG_MATERIAL_TEXTURE("cubemap", sfg_texturecube)
SFG_MATERIAL_SAMPLER("cubemap")

struct vs_output
{
	float4 pos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

static const float3 SFG_SKYBOX_VERTICES[36] = {
	float3(-1.0, -1.0, -1.0), float3(-1.0, 1.0, -1.0), float3(1.0, 1.0, -1.0),
	float3(1.0, 1.0, -1.0), float3(1.0, -1.0, -1.0), float3(-1.0, -1.0, -1.0),
	float3(-1.0, -1.0, 1.0), float3(1.0, -1.0, 1.0), float3(1.0, 1.0, 1.0),
	float3(1.0, 1.0, 1.0), float3(-1.0, 1.0, 1.0), float3(-1.0, -1.0, 1.0),
	float3(-1.0, 1.0, -1.0), float3(-1.0, 1.0, 1.0), float3(1.0, 1.0, 1.0),
	float3(1.0, 1.0, 1.0), float3(1.0, 1.0, -1.0), float3(-1.0, 1.0, -1.0),
	float3(-1.0, -1.0, -1.0), float3(1.0, -1.0, -1.0), float3(1.0, -1.0, 1.0),
	float3(1.0, -1.0, 1.0), float3(-1.0, -1.0, 1.0), float3(-1.0, -1.0, -1.0),
	float3(-1.0, -1.0, 1.0), float3(-1.0, 1.0, 1.0), float3(-1.0, 1.0, -1.0),
	float3(-1.0, 1.0, -1.0), float3(-1.0, -1.0, -1.0), float3(-1.0, -1.0, 1.0),
	float3(1.0, -1.0, -1.0), float3(1.0, 1.0, -1.0), float3(1.0, 1.0, 1.0),
	float3(1.0, 1.0, 1.0), float3(1.0, -1.0, 1.0), float3(1.0, -1.0, -1.0)
};

vs_output VSMain(uint vertex_id : SV_VertexID)
{
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	vs_output output;
	output.direction = SFG_SKYBOX_VERTICES[vertex_id];
	const float4 clip = mul(view_data.view_proj, float4(output.direction, 0.0));
	output.pos = float4(clip.xy, 0.0, clip.w);
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	render_pass_data_lighting lighting_data = sfg_get_cbv<render_pass_data_lighting>(SFG_RENDER_PASS_LIGHTING);
	TextureCube cubemap = sfg_get_texture<TextureCube>(sfg_constant_mat1);
	SamplerState cubemap_sampler = sfg_get_sampler_state(sfg_constant_mat2);
	return float4(cubemap.Sample(cubemap_sampler, normalize(input.direction)).rgb * lighting_data.environment_intensity, 1.0);
}
