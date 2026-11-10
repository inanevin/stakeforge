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
#if defined(UI_WORLD_SPACE)
#include "entity.hlsl"
#include "render_pass_defines.hlsl"

struct ui_canvas_data
{
    float2 origin;
    float units_per_pixel;
    uint entity_index;
    uint first_parameter;
};
#endif

struct projection_cb
{
    float4x4 projection;
};

struct ui_draw_parameters
{
    float4 clip_bounds;
    uint4 constants;
    uint canvas_index;
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
#if defined(UI_WORLD_SPACE)
    StructuredBuffer<ui_canvas_data> canvas_data = sfg_get_ssbo<ui_canvas_data>(sfg_constant_rp2);
    ConstantBuffer<render_pass_data_view> view_data = sfg_get_cbv<render_pass_data_view>(sfg_constant_rp3);
    StructuredBuffer<gpu_entity> entities = sfg_get_ssbo<gpu_entity>(sfg_constant_rp4);
    ui_canvas_data canvas = canvas_data[item.canvas_index];
    float2 local = (canvas.origin - IN.pos) * canvas.units_per_pixel;
    OUT.pos = mul(view_data.view_proj, mul(entities[canvas.entity_index].model, float4(local, 0.0f, 1.0f)));
    OUT.pos.z *= 1.0f + float(IN.parameter_index - canvas.first_parameter) * 0.000001f;
#else
    OUT.pos = mul(projection.projection, float4(IN.pos, 0.0f, 1.0f));
#endif
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

float4 ui_output_color(float4 color)
{
#if defined(UI_WORLD_SPACE)
    clip(color.a - 0.001f);
#endif
    return color;
}
