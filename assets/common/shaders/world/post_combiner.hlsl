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

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct render_pass_data
{
	float4 params0;
	float4 params1;
};

SamplerState smp_linear : static_sampler_linear;

vs_output VSMain(uint vertex_id : SV_VertexID)
{
	vs_output output;

	float2 pos;
	if (vertex_id == 0)
		pos = float2(-1.0, -1.0);
	else if (vertex_id == 1)
		pos = float2(-1.0, 3.0);
	else
		pos = float2(3.0, -1.0);

	output.pos = float4(pos, 0.0, 1.0);
	output.uv	= pos * float2(0.5, -0.5) + 0.5;
	return output;
}

float3 white_balance(float3 color, float temperature, float tint)
{
	float3 gains = float3(1.0 + temperature * 0.1 - tint * 0.05, 1.0 + tint * 0.1, 1.0 - temperature * 0.1 - tint * 0.05);
	return max(color * gains, 0.0);
}

float3 adjust_saturation(float3 color, float saturation)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	return lerp(luminance.xxx, color, saturation);
}

float3 aces_fitted(float3 color)
{
	float3 a = color * (color + 0.0245786) - 0.000090537;
	float3 b = color * (0.983729 * color + 0.432951) + 0.238081;
	return saturate(a / b);
}

float3 reinhard_extended(float3 color, float white_point)
{
	float white_squared = white_point * white_point;
	return saturate(color * (1.0 + color / white_squared) / (1.0 + color));
}

float4 PSMain(vs_output input) : SV_TARGET
{
	render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_constant_rp0);
	Texture2D<float4> lighting_texture = sfg_get_texture<Texture2D<float4> >(sfg_constant_rp1);
	Texture2D<float4> bloom_texture = sfg_get_texture<Texture2D<float4> >(sfg_constant_rp2);
	float3 color = lighting_texture.SampleLevel(smp_linear, input.uv, 0).rgb;
	color += bloom_texture.SampleLevel(smp_linear, input.uv, 0).rgb * rp_data.params0.x;
	color *= exp2(rp_data.params0.y);
	color = white_balance(max(color, 0.0), rp_data.params1.x, rp_data.params1.y);
	color = adjust_saturation(color, rp_data.params0.z);
	uint tonemap_mode = (uint)rp_data.params1.z;
	if (tonemap_mode == 0)
		color = aces_fitted(color);
	else if (tonemap_mode == 1)
		color = reinhard_extended(color, max(rp_data.params0.w, 0.001));
	else
		color = saturate(color);
	return float4(color, 1.0);
}
