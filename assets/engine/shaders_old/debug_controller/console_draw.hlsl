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
// Pixel Shader: just output the interpolated vertex color
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	float2 uv = IN.uv;

	Texture2D txt_render_target = sfg_get_texture<Texture2D>(sfg_object_constant0);
	float2 screen_size = float2(float(sfg_object_constant1), float(sfg_object_constant2));
    // --- Barrel distortion ---
	const float distortionAmount = 0.0015f;
	const float baseAberration = 0.002; 
	
	float2 centeredUV = uv * 2.0f - 1.0f;
	float r2 = dot(centeredUV, centeredUV);
	centeredUV += centeredUV * r2 * float2(distortionAmount, 0.0);

    // --- Chromatic aberration (edge only) ---
	float edgeFactor = saturate((r2 - 0.85));
    // 0.0 near center, 1.0 near corners � tweak the 0.2/0.8

	float2 aberration_offset = centeredUV * baseAberration * edgeFactor;
	const float2 uvR = (centeredUV + aberration_offset) * 0.5f + 0.5f;
	const float2 uvG = centeredUV * 0.5f + 0.5f;
	const float2 uvB = (centeredUV - aberration_offset) * 0.5f + 0.5f;
	float4 sample_red = txt_render_target.SampleLevel(sampler_base, uvR, 0);
	float red = sample_red.r;
	float green = txt_render_target.SampleLevel(sampler_base, uvG, 0).g;
	float blue = txt_render_target.SampleLevel(sampler_base, uvB, 0).b;
	float3 color = float3(red, green, blue);
	
	float falloff = dot(centeredUV, centeredUV); // r^2 falloff
	float shading = saturate(1.0 - falloff * 0.15); // tweak 0.75 for how fast it darkens
	color.rgb *= shading;
	
    // --- Scanlines ---
	float pixel_y = IN.uv.y * screen_size.y;
	if ((int) pixel_y % 3 == 0)
	{
		color.rgb *= 0.9;
	}

	return float4(color.rgb, sample_red.a);
}