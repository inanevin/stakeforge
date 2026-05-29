#include "layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

SamplerState samp_linear : static_sampler_linear;

struct VSInput
{
	float2 pos   : POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

VSOutput VSMain(VSInput IN)
{
	ConstantBuffer<projection_cb> proj = sfg_get_cbv<projection_cb>(sfg_constant_rp0);
	VSOutput OUT;
	OUT.pos = mul(proj.projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv  = IN.uv;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D source_texture = sfg_get_texture<Texture2D>(sfg_constant_obj0);
	return source_texture.Sample(samp_linear, IN.uv);
}
