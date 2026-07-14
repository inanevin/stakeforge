#include "layout_defines.hlsl"

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

SamplerState smp_linear : static_sampler_linear;

struct editor_world_render_data
{
	float4 params;
	float4 selection_color;
};

vs_output VSMain(uint vertex_id : SV_VertexID)
{
	vs_output output;
	float2	  pos;
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

float4 PSMain(vs_output input) : SV_TARGET
{
	editor_world_render_data data = sfg_get_cbv<editor_world_render_data>(sfg_constant_rp0);
	Texture2D source = sfg_get_texture<Texture2D>(sfg_constant_obj0);
	float4 color = source.SampleLevel(smp_linear, input.uv, 0);
	if (data.params.w <= 0.0)
		return color;

	Texture2D selection = sfg_get_texture<Texture2D>(sfg_constant_obj1);
	float2 texel = 1.0 / data.params.xy;
	float thickness = data.params.z;
	float2 diff_uv = texel * thickness;
	float4 x_diff = selection.SampleLevel(smp_linear, input.uv - float2(diff_uv.x, 0.0), 0);
	x_diff -= selection.SampleLevel(smp_linear, input.uv + float2(diff_uv.x, 0.0), 0);
	float4 y_diff = selection.SampleLevel(smp_linear, input.uv - float2(0.0, diff_uv.y), 0);
	y_diff -= selection.SampleLevel(smp_linear, input.uv + float2(0.0, diff_uv.y), 0);
	float outline = saturate(length(sqrt(x_diff * x_diff + y_diff * y_diff).rgb));
	return float4(saturate(color.rgb + data.selection_color.rgb * outline), color.a);
}
