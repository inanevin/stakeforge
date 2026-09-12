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

#include "post_process.hlsl"

SFG_MATERIAL_PARAM_F32("intensity", 0.65, 0.0, 1.0)
SFG_MATERIAL_PARAM_F32("radius", 0.75, 0.0, 2.0)
SFG_MATERIAL_PARAM_F32("softness", 0.35, 0.001, 1.0)

struct material_data
{
	float intensity;
	float radius;
	float softness;
};

post_process_vs_output VSMain(uint vertex_id : SV_VertexID)
{
	return sfg_post_process_fullscreen_vertex(vertex_id);
}

float4 PSMain(post_process_vs_output input) : SV_TARGET
{
	const material_data material = sfg_get_cbv<material_data>(sfg_constant_mat0);
	const render_pass_data_view view_data = sfg_post_process_get_view();
	float4 color = sfg_post_process_sample_source(input.uv);

	float2 centered_uv = input.uv * 2.0 - 1.0;
	centered_uv.x *= view_data.viewport_size.x / view_data.viewport_size.y;

	const float edge = 1.0 - smoothstep(material.radius, material.radius + material.softness, length(centered_uv));
	color.rgb *= lerp(1.0, edge, material.intensity);
	return color;
}
