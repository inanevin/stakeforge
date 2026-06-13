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

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct VSInput
{
	float3 pos : POSITION; 
	float3 next_pos : POSITION1; 
	float4 color : COLOR0; 
	float dist : POSITION2;
};


struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

struct render_pass_data
{
	float4x4 view;
	float4x4 proj;
	float4x4 view_proj;
	float4 cam_right_and_pixel_size;
	float4 cam_up;
	float4 resolution_and_planes;
	float sdf_thickness;
	float sdf_softness;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	render_pass_data rp = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

	float4 pos = float4(IN.pos, 1.0);
	float4 next_pos = float4(IN.next_pos, 1.0);

	float4 current_view = mul(rp.view, pos);
	float aspect = rp.resolution_and_planes.x / rp.resolution_and_planes.y;

	float4 current_proj = mul(rp.view_proj, pos);
	float4 next_proj = mul(rp.view_proj, next_pos);

	float2 current_screen = (current_proj.xy / current_proj.w) * aspect;
	float2 next_screen = (next_proj.xy / next_proj.w) * aspect;

	float2 dir = normalize(next_screen - current_screen);
	float2 normal = float2(-dir.y, dir.x);

	float base_thickness = 0.25;
	float thickness_alpha = (current_proj.z / rp.resolution_and_planes.w);
	float fac = lerp(0.0, 0.2, thickness_alpha);
	float thickness = base_thickness + fac;
	normal *= thickness;

	float4 offset = float4(normal * IN.dist, 0.0, 0.0);
	OUT.pos = current_proj + offset;
	OUT.color = IN.color;

	return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader: just output the interpolated vertex color
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	return IN.color;
}