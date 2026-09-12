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

Texture2D _txt_render_target : material_texture0;
SamplerState _smp_base : static_sampler_linear;

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	float4 color = _txt_render_target.SampleLevel(_smp_base, IN.uv, 0);
	return color;
}