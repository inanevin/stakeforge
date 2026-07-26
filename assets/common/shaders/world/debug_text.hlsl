#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"

SamplerState smp : static_sampler_nearest;

struct vs_input
{
	float3 anchor : POSITION0;
	float2 offset : POSITION1;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float mode : TEXCOORD1;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	nointerpolation float mode : TEXCOORD1;
	float clip_distance : SV_ClipDistance0;
};

struct render_pass_data_debug_text
{
	float4 params;
};

vs_output VSMain(vs_input input)
{
	vs_output output = (vs_output)0;
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	
	if (input.mode < 0.5)
	{
		float2 pixel_position = input.anchor.xy + input.offset;
		output.pos = float4(pixel_position.x * 2.0 / view_data.viewport_size.x - 1.0, 1.0 - pixel_position.y * 2.0 / view_data.viewport_size.y, 0.0, 1.0);
		output.clip_distance = 1.0;
	}
	else
	{
		float4 clip_position = mul(view_data.view_proj, float4(input.anchor, 1.0));
		clip_position.xy += input.offset * float2(2.0 / view_data.viewport_size.x, -2.0 / view_data.viewport_size.y) * clip_position.w;
		output.pos = clip_position;
		output.clip_distance = clip_position.w > 0.0 ? 1.0 : -1.0;
	}
	
	output.uv = input.uv;
	output.color = input.color;
	output.mode = input.mode;
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	Texture2D atlas = sfg_get_texture<Texture2D>(sfg_constant_mat0);
	float coverage = atlas.SampleLevel(smp, input.uv, 0).r;
	
	clip(coverage - 0.001);
	
	if (input.mode > 0.5 && input.mode < 1.5)
	{
		render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
		render_pass_data_debug_text data = sfg_get_cbv<render_pass_data_debug_text>(SFG_RENDER_PASS_SPECIFIC);
		Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(view_data.depth_texture_index);
		
		float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));
		
		if (scene_depth > 0.000001 && input.pos.z + data.params.x < scene_depth)
			discard;
	}
	
	return float4(input.color.rgb, input.color.a * coverage);
}
