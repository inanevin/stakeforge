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
	float2 pos : POSITION; // XY position
	float2 uv : TEXCOORD0; // UV coords (unused in PS here, but available)
	float4 color : COLOR0; // RGBA vertex color
};


struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float2 uv : TEXCOORD0; // pass-through UV
	float4 color : COLOR0; // pass-through color
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------


#if IS_3D

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

#else

struct render_pass_data
{
	float4x4 projection;
	float sdf_thickness;
	float sdf_softness;
};

#endif


#if IS_3D
struct draw_data
{
	float4 position_and_size;
};
#endif

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	render_pass_data rp_ubo = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

#if IS_3D
	float4 worldPos = float4(IN.pos, 0.0f, 1.0f);
	uint draw_index = sfg_mat_constant0;
	StructuredBuffer<draw_data> draw_data_buffer = sfg_get_ssbo<draw_data>(sfg_mat_constant1);
	draw_data dd = draw_data_buffer[draw_index];

    float3 anchor_ws = dd.position_and_size.xyz;
    float  scale_ws  = dd.position_and_size.w;

    float2 local_ws_2d = (IN.pos * rp_ubo.cam_right_and_pixel_size.w) * scale_ws;

    float3 world_pos =
        anchor_ws +
        rp_ubo.cam_right_and_pixel_size.xyz * local_ws_2d.x +
        rp_ubo.cam_up.xyz    * local_ws_2d.y;

    OUT.pos = mul(rp_ubo.view_proj, float4(world_pos, 1.0f));

#else
	float4 worldPos = float4(IN.pos, 0.0f, 1.0f);
	OUT.pos = mul(rp_ubo.projection, worldPos);
#endif

	OUT.uv = IN.uv;
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