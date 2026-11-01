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

#define DEBUG_TEXTURE_FLAG_DEPTH_TESTED 1
#define DEBUG_TEXTURE_FLAG_LINEAR_SAMPLE 2
#define DEBUG_TEXTURE_DEPTH_BIAS 0.00005

SamplerState smp_linear : static_sampler_linear;
SamplerState smp_nearest : static_sampler_nearest;

struct debug_texture
{
	float4 color;
	float3 position;
	uint texture_index;
	float2 size_px;
	float2 screen_offset;
	uint entity_id;
	uint flags;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	nointerpolation uint texture_index : TEXCOORD1;
	nointerpolation uint entity_id : TEXCOORD2;
	nointerpolation uint flags : TEXCOORD3;
	float clip_distance : SV_ClipDistance0;
};

vs_output VSMain(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	static const float2 positions[6] = {
		float2(-0.5, -0.5),
		float2(-0.5, 0.5),
		float2(0.5, 0.5),
		float2(-0.5, -0.5),
		float2(0.5, 0.5),
		float2(0.5, -0.5)
	};
	static const float2 uvs[6] = {
		float2(0.0, 0.0),
		float2(0.0, 1.0),
		float2(1.0, 1.0),
		float2(0.0, 0.0),
		float2(1.0, 1.0),
		float2(1.0, 0.0)
	};

	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	StructuredBuffer<debug_texture> texture_buffer = sfg_get_ssbo<debug_texture>(sfg_constant_obj0);
	debug_texture instance = texture_buffer[instance_id];
	float4 clip_position = mul(view_data.view_proj, float4(instance.position, 1.0));
	float2 pixel_offset = positions[vertex_id] * instance.size_px + instance.screen_offset;
	clip_position.xy += pixel_offset * float2(2.0 / view_data.viewport_size.x, -2.0 / view_data.viewport_size.y) * clip_position.w;

	vs_output output = (vs_output)0;
	output.pos = clip_position;
	output.uv = uvs[vertex_id];
	output.color = instance.color;
	output.texture_index = instance.texture_index;
	output.entity_id = instance.entity_id;
	output.flags = instance.flags;
	output.clip_distance = clip_position.w > 0.0 ? 1.0 : -1.0;
	return output;
}

float4 sample_debug_texture(vs_output input)
{
	Texture2D texture = sfg_get_texture_non_uniform<Texture2D>(input.texture_index);
	float4 color = (input.flags & DEBUG_TEXTURE_FLAG_LINEAR_SAMPLE) != 0 ? texture.Sample(smp_linear, input.uv) : texture.Sample(smp_nearest, input.uv);
	color *= input.color;
	clip(color.a - 0.001);
	return color;
}

void test_debug_texture_depth(vs_output input)
{
	if ((input.flags & DEBUG_TEXTURE_FLAG_DEPTH_TESTED) == 0)
		return;

	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(view_data.depth_texture_index);
	float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));

	if (scene_depth > 0.000001 && input.pos.z + DEBUG_TEXTURE_DEPTH_BIAS < scene_depth)
		discard;
}

#ifdef WRITE_ID

uint PSMain(vs_output input) : SV_TARGET
{
	sample_debug_texture(input);

	if (input.entity_id == 0xffffffff)
		discard;

	test_debug_texture_depth(input);
	return input.entity_id;
}

#else

float4 PSMain(vs_output input) : SV_TARGET
{
	float4 color = sample_debug_texture(input);
	test_debug_texture_depth(input);
	return color;
}

#endif
