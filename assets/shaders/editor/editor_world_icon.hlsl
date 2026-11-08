/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "layout_defines.hlsl"

#define EDITOR_ICON_DEPTH_BIAS 0.00005
#define EDITOR_ICON_OCCLUDED_ALPHA 0.25

SamplerState smp_linear : static_sampler_linear;

struct editor_icon
{
	float4 clip_position;
	float2 size_px;
	float2 screen_offset;
	uint texture_index;
	uint entity_id;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	nointerpolation uint texture_index : TEXCOORD1;
	nointerpolation uint entity_id : TEXCOORD2;
	float clip_distance : SV_ClipDistance0;
};

vs_output VSMain(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	static const float2 positions[6] = {
		float2(-0.5, -0.5), float2(-0.5, 0.5), float2(0.5, 0.5),
		float2(-0.5, -0.5), float2(0.5, 0.5), float2(0.5, -0.5)
	};
	static const float2 uvs[6] = {
		float2(0.0, 0.0), float2(0.0, 1.0), float2(1.0, 1.0),
		float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0)
	};

	StructuredBuffer<editor_icon> icons = sfg_get_ssbo<editor_icon>(sfg_constant_rp0);
	editor_icon icon = icons[instance_id];
	float2 pixel_offset = positions[vertex_id] * icon.size_px + icon.screen_offset;
	float4 clip_position = icon.clip_position;
	clip_position.xy += pixel_offset * float2(2.0 / float(sfg_constant_rp2), -2.0 / float(sfg_constant_rp3)) * clip_position.w;

	vs_output output = (vs_output)0;
	output.pos = clip_position;
	output.uv = uvs[vertex_id];
	output.texture_index = icon.texture_index;
	output.entity_id = icon.entity_id;
	output.clip_distance = clip_position.w > 0.0 ? 1.0 : -1.0;
	return output;
}

#ifdef WRITE_ID
uint PSMain(vs_output input) : SV_TARGET
#else
float4 PSMain(vs_output input) : SV_TARGET
#endif
{
	Texture2D texture = sfg_get_texture_non_uniform<Texture2D>(input.texture_index);
	float4 color = texture.Sample(smp_linear, input.uv);
	clip(color.a - 0.001);

#ifdef WRITE_ID
	return input.entity_id;
#else
	Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(sfg_constant_rp1);
	float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));

	if (scene_depth > 0.000001 && input.pos.z + EDITOR_ICON_DEPTH_BIAS < scene_depth)
		color.a *= EDITOR_ICON_OCCLUDED_ALPHA;

	return color;
#endif
}
