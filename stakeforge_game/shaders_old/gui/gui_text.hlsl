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
	float2 pos : POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
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

SamplerState sampler_base : static_sampler_gui_text;

//------------------------------------------------------------------------------
// Pixel Shader: just output the interpolated vertex color
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D txt_atlas = sfg_get_texture<Texture2D>(sfg_mat_constant2);

	float4 tex_color = txt_atlas.SampleLevel(sampler_base, IN.uv, 0);
	return float4(IN.color.xyz, tex_color.r * IN.color.w);
}