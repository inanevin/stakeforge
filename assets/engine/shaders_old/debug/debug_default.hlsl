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
	float3 pos : POSITION; // XY position
	float4 color : COLOR0; // RGBA vertex color
};


struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float4 color : COLOR0; // pass-through color
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
	float4 resolution;
	float sdf_thickness;
	float sdf_softness;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	render_pass_data rp = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

    OUT.pos = mul(rp.view_proj, float4(IN.pos, 1.0f));
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