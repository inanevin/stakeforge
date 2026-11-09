/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "layout_defines.hlsl"

struct projection_cb
{
    float4x4 projection;
};

struct ui_draw_parameters
{
    float4 clip_bounds;
    uint4 constants;
};

struct VSInput
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint parameter_index : TEXCOORD1;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float2 pixel : TEXCOORD1;
    nointerpolation uint4 constants : TEXCOORD2;
    float4 clip_distance : SV_ClipDistance0;
};

VSOutput VSMain(VSInput IN)
{
    ConstantBuffer<projection_cb> projection = sfg_get_cbv<projection_cb>(sfg_constant_rp0);
    StructuredBuffer<ui_draw_parameters> parameters = sfg_get_ssbo<ui_draw_parameters>(sfg_constant_rp1);
    ui_draw_parameters item = parameters[IN.parameter_index];
    VSOutput OUT = (VSOutput)0;
    OUT.pos = mul(projection.projection, float4(IN.pos, 0.0f, 1.0f));
    OUT.uv = IN.uv;
    OUT.color = IN.color;
    OUT.pixel = IN.pos;
    OUT.constants = item.constants;
    OUT.clip_distance = float4(IN.pos - item.clip_bounds.xy, item.clip_bounds.zw - IN.pos);
    return OUT;
}

SamplerState ui_text_nearest : static_sampler_nearest;
SamplerState ui_text_linear : static_sampler_linear;

float3 ui_text_coverage(Texture2D atlas, float2 uv)
{
#if defined(UI_TEXT_LCD)
	float3 coverage = atlas.SampleLevel(ui_text_nearest, uv, 0).rgb;
	return pow(coverage, 1.0f / 1.43f);
#elif defined(UI_TEXT_SDF)
	float distance = atlas.SampleLevel(ui_text_linear, uv, 0).r;
	float width = max(0.0625f, fwidth(distance));
	return smoothstep(0.5f - width, 0.5f + width, distance).xxx;
#else
	return atlas.SampleLevel(ui_text_nearest, uv, 0).rrr;
#endif
}

float4 ui_text_color(float3 coverage, float4 color)
{
#if defined(UI_TEXT_LCD)
	float alpha = max(coverage.r, max(coverage.g, coverage.b));
	return float4(color.rgb * coverage * color.a, color.a * alpha);
#else
	return float4(color.rgb, color.a * coverage.r);
#endif
}
