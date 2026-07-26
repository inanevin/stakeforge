#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"

struct vs_input
{
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

struct editor_gizmo_data
{
	float4x4 models[7];
	float4 colors[7];
	float4 offsets[7];
	float4 params;
};

vs_output VSMain(vs_input input)
{
	vs_output output;
	render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	editor_gizmo_data gizmo_data = sfg_get_cbv<editor_gizmo_data>(SFG_RENDER_PASS_SPECIFIC);
	uint axis = sfg_constant_obj0;
	float3 pivot = mul(gizmo_data.models[axis], float4(0.0, 0.0, 0.0, 1.0)).xyz;
	float4 pivot_clip = mul(view_data.view_proj, float4(pivot, 1.0));
	float world_scale = max(gizmo_data.params.x * 2.0 * pivot_clip.w * gizmo_data.params.z / gizmo_data.params.y, gizmo_data.params.w) * float(sfg_constant_obj1) * 0.01;
	float3 world_pos = mul(gizmo_data.models[axis], float4(input.pos * world_scale, 1.0)).xyz + gizmo_data.offsets[axis].xyz * world_scale;
	output.pos = mul(view_data.view_proj, float4(world_pos, 1.0));
	output.color = gizmo_data.colors[axis];
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	return input.color;
}
