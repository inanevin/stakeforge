// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//
//  Author: Inan Evin
//  http://www.inanevin.com
//
//  Copyright (c) [2025-] [Inan Evin]
//
//  Stakeforge is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  Stakeforge is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.
//
//  As an additional permission under section 7 of GPLv3, the copyright
//  holders grant the Stakeforge Game Linking Exception, version 1.0,
//  in GAME-LINKING-EXCEPTION.md.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"
#include "normal.hlsl"
#include "depth.hlsl"

#define SFG_POST_PROCESS_VIEW sfg_constant_rp0
#define SFG_POST_PROCESS_SOURCE sfg_constant_rp1
#define SFG_POST_PROCESS_GBUFFER_ALBEDO sfg_constant_rp2
#define SFG_POST_PROCESS_GBUFFER_NORMAL sfg_constant_rp3
#define SFG_POST_PROCESS_GBUFFER_ORM sfg_constant_rp4
#define SFG_POST_PROCESS_GBUFFER_EMISSIVE sfg_constant_rp5

struct post_process_vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

SamplerState sfg_post_process_sampler_linear : static_sampler_linear;

post_process_vs_output sfg_post_process_fullscreen_vertex(uint vertex_id)
{
	post_process_vs_output output;

	float2 pos;
	if (vertex_id == 0)
		pos = float2(-1.0, -1.0);
	else if (vertex_id == 1)
		pos = float2(-1.0, 3.0);
	else
		pos = float2(3.0, -1.0);

	output.pos = float4(pos, 0.0, 1.0);
	output.uv = pos * float2(0.5, -0.5) + 0.5;
	return output;
}

render_pass_data_view sfg_post_process_get_view()
{
	return sfg_get_cbv<render_pass_data_view>(SFG_POST_PROCESS_VIEW);
}

float4 sfg_post_process_sample_source(float2 uv)
{
	Texture2D<float4> source = sfg_get_texture<Texture2D<float4> >(SFG_POST_PROCESS_SOURCE);
	return source.SampleLevel(sfg_post_process_sampler_linear, uv, 0);
}

float4 sfg_post_process_load_albedo(uint2 pixel)
{
	Texture2D<float4> albedo = sfg_get_texture<Texture2D<float4> >(SFG_POST_PROCESS_GBUFFER_ALBEDO);
	return albedo.Load(int3(pixel, 0));
}

float3 sfg_post_process_load_normal(uint2 pixel)
{
	Texture2D<float2> normal = sfg_get_texture<Texture2D<float2> >(SFG_POST_PROCESS_GBUFFER_NORMAL);
	return oct_decode(normal.Load(int3(pixel, 0)));
}

float3 sfg_post_process_load_orm(uint2 pixel)
{
	Texture2D<float4> orm = sfg_get_texture<Texture2D<float4> >(SFG_POST_PROCESS_GBUFFER_ORM);
	return orm.Load(int3(pixel, 0)).rgb;
}

float3 sfg_post_process_load_emissive(uint2 pixel)
{
	Texture2D<float4> emissive = sfg_get_texture<Texture2D<float4> >(SFG_POST_PROCESS_GBUFFER_EMISSIVE);
	return emissive.Load(int3(pixel, 0)).rgb;
}

float sfg_post_process_load_depth(uint2 pixel)
{
	const render_pass_data_view view_data = sfg_post_process_get_view();
	Texture2D<float> depth = sfg_get_texture<Texture2D<float> >(view_data.depth_texture_index);
	return depth.Load(int3(pixel, 0));
}

float3 sfg_post_process_reconstruct_world_position(float2 uv, float depth)
{
	const render_pass_data_view view_data = sfg_post_process_get_view();
	return reconstruct_world_position(uv, depth, view_data.inv_view_proj);
}
