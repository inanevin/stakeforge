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