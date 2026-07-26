#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"

struct vs_input
{
	float3 position : POSITION0;
	float4 color : COLOR0;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

vs_output VSMain(vs_input input)
{
	vs_output output;
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	output.pos = mul(view_data.view_proj, float4(input.position, 1.0));
	output.color = input.color;
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(view_data.depth_texture_index);
	float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));

	if (scene_depth > 0.000001 && input.pos.z + 0.00005 < scene_depth)
		discard;

	return input.color;
}
