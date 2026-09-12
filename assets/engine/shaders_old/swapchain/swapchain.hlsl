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
#include "../layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float2 uv : TEXCOORD0; // pass-through UV
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

VSOutput VSMain(uint vertexID : SV_VertexID)
{
	VSOutput OUT;
	
	float2 pos = float2(
        (vertexID == 2) ? 3.0 : -1.0,
        (vertexID == 1) ? -3.0 : 1.0
    );
	
	OUT.pos = float4(pos, 0.0f, 1.0f);
	OUT.uv = 0.5f * (pos + 1.0f);
	OUT.uv.y = 1.0f - OUT.uv.y;
	return OUT;
}

SamplerState sampler_base : static_sampler_linear;

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{

#ifdef SFG_CONSOLE_AND_EDITOR
	Texture2D txt_world = sfg_get_texture<Texture2D>(sfg_rp_constant0);
	Texture2D txt_debug_controller = sfg_get_texture<Texture2D>(sfg_rp_constant1);
	Texture2D txt_editor = sfg_get_texture<Texture2D>(sfg_rp_constant2);

	float2 debug_controller_uv = float2(IN.uv.x, IN.uv.y * 2);
	float4 color_debug_controller = txt_debug_controller.SampleLevel(sampler_base, debug_controller_uv, 0);
	float4 color_world = txt_world.SampleLevel(sampler_base, IN.uv, 0);
	float4 color_editor = txt_editor.SampleLevel(sampler_base, IN.uv, 0);

	if(IN.uv.y < 0.5)
	{
		float4 c = color_world * (1.0f - color_editor.w) + color_editor;
		return (c * (1.0f - color_debug_controller.w)) + color_debug_controller;
	}

	return color_world * (1.0 - color_editor.w) + color_editor;

#elif SFG_CONSOLE

	Texture2D txt_world = sfg_get_texture<Texture2D>(sfg_rp_constant0);
	Texture2D txt_debug_controller = sfg_get_texture<Texture2D>(sfg_rp_constant1);

	float2 debug_controller_uv = float2(IN.uv.x, IN.uv.y * 2);
	float4 color_debug_controller = txt_debug_controller.SampleLevel(sampler_base, debug_controller_uv, 0);
	float4 color_world = txt_world.SampleLevel(sampler_base, IN.uv, 0);

	if(IN.uv.y < 0.5)
	{
		return (color_world * (1.0f - color_debug_controller.w)) + color_debug_controller;
	}

	return color_world;
#else

	Texture2D txt_world = sfg_get_texture<Texture2D>(sfg_rp_constant0);
	return txt_world.SampleLevel(sampler_base, IN.uv, 0);
#endif
}