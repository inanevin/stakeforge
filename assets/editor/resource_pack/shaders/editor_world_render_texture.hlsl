#include "layout_defines.hlsl"

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

SamplerState smp_linear : static_sampler_linear;

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
	Texture2D source = sfg_get_texture<Texture2D>(sfg_constant_obj0);
	return source.SampleLevel(smp_linear, input.uv, 0);
}
