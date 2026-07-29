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
	float4 params2;
	float4 params3;
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

float get_luma(float3 color)
{
	return sqrt(max(dot(color, float3(0.2126, 0.7152, 0.0722)), 0.0));
}

float4 PSMain(vs_output input) : SV_TARGET
{
	const render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_constant_rp0);
	Texture2D<float4> source_texture = sfg_get_texture<Texture2D<float4> >(sfg_constant_rp1);
	const float2 texel_size = rp_data.params2.xy;
	const float fixed_threshold = rp_data.params2.z;
	const float relative_threshold = rp_data.params2.w;
	const float subpixel_blending = rp_data.params3.x;
	const float4 center = source_texture.SampleLevel(smp_linear, input.uv, 0);
	const float luma_center = get_luma(center.rgb);
	const float luma_north = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(0.0, -texel_size.y), 0).rgb);
	const float luma_south = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(0.0, texel_size.y), 0).rgb);
	const float luma_west = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(-texel_size.x, 0.0), 0).rgb);
	const float luma_east = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(texel_size.x, 0.0), 0).rgb);
	const float luma_min = min(luma_center, min(min(luma_north, luma_south), min(luma_west, luma_east)));
	const float luma_max = max(luma_center, max(max(luma_north, luma_south), max(luma_west, luma_east)));
	const float contrast = luma_max - luma_min;

	if (contrast < max(fixed_threshold, relative_threshold * luma_max))
		return center;

	const float luma_north_west = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(-texel_size.x, -texel_size.y), 0).rgb);
	const float luma_north_east = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(texel_size.x, -texel_size.y), 0).rgb);
	const float luma_south_west = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(-texel_size.x, texel_size.y), 0).rgb);
	const float luma_south_east = get_luma(source_texture.SampleLevel(smp_linear, input.uv + float2(texel_size.x, texel_size.y), 0).rgb);
	float2 direction = float2(
		-(luma_north_west + luma_north_east - luma_south_west - luma_south_east),
		luma_north_west + luma_south_west - luma_north_east - luma_south_east);
	const float direction_reduce = max((luma_north_west + luma_north_east + luma_south_west + luma_south_east) * 0.03125, 0.0078125);
	const float direction_scale = 1.0 / (min(abs(direction.x), abs(direction.y)) + direction_reduce);
	direction = clamp(direction * direction_scale, -8.0, 8.0) * texel_size;

	const float3 color_a = 0.5 * (
		source_texture.SampleLevel(smp_linear, input.uv + direction * -0.1666667, 0).rgb +
		source_texture.SampleLevel(smp_linear, input.uv + direction * 0.1666667, 0).rgb);
	const float3 color_b = color_a * 0.5 + 0.25 * (
		source_texture.SampleLevel(smp_linear, input.uv + direction * -0.5, 0).rgb +
		source_texture.SampleLevel(smp_linear, input.uv + direction * 0.5, 0).rgb);
	const float luma_b = get_luma(color_b);
	const float3 filtered = luma_b < luma_min || luma_b > luma_max ? color_a : color_b;
	return float4(lerp(center.rgb, filtered, subpixel_blending), center.a);
}
