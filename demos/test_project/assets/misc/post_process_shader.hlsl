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
