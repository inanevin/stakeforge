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
#include "fog.hlsl"

SFG_MATERIAL_PARAM_VEC4("zenith_color", sfg_color)
SFG_MATERIAL_PARAM_VEC4("horizon_color", sfg_color)
SFG_MATERIAL_PARAM_VEC4("ground_color", sfg_color)

struct material_data
{
	float4 zenith_color;
	float4 horizon_color;
	float4 ground_color;
};

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
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	render_pass_data_lighting lighting_data = sfg_get_cbv<render_pass_data_lighting>(SFG_RENDER_PASS_LIGHTING);
	render_pass_data_fog fog_data = sfg_get_cbv<render_pass_data_fog>(SFG_RENDER_PASS_FOG);
	material_data material = sfg_get_cbv<material_data>(sfg_constant_mat0);
	const float3 direction = normalize(input.direction);
	const float horizon = direction.y;
	float3 color = horizon >= 0.0
		? lerp(material.horizon_color.rgb, material.zenith_color.rgb, saturate(horizon))
		: lerp(material.horizon_color.rgb, material.ground_color.rgb, saturate(-horizon));
	color *= lighting_data.environment_intensity;

	if ((view_data.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG) != 0)
		color = apply_fog(color, view_data.camera_pos.xyz + direction * view_data.far_plane, view_data.camera_pos.xyz, fog_data);

	return float4(color, 1.0);
}
