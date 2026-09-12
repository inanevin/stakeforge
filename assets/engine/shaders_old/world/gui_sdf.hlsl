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

struct render_pass_data
{
	float4x4 projection;
	float2 resolution;
};

struct mat_data
{
	float sdf_thickness;
	float sdf_softness;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
    
	render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

	float4 worldPos = float4(IN.pos, 0.0f, 1.0f);
	OUT.pos = mul(rp_data.projection, worldPos);
	OUT.uv = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------

SamplerState sampler_base : static_sampler_gui_text;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	mat_data mat = sfg_get_cbv<mat_data>(sfg_mat_constant0);
	Texture2D txt_atlas = sfg_get_texture<Texture2D>(sfg_mat_constant2);
	
	float distance = txt_atlas.SampleLevel(sampler_base, IN.uv, 0).x;
	float alpha = smoothstep(mat.sdf_thickness - mat.sdf_softness, mat.sdf_thickness + mat.sdf_softness, distance);
	return float4(IN.color.xyz, alpha);
}