#include "layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

SamplerState smp : static_sampler_nearest;

struct VSInput
{
	float2 pos   : POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

struct VSOutput
{
	float4 pos   : SV_POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	ConstantBuffer<projection_cb> proj = sfg_get_cbv<projection_cb>(sfg_constant_rp0);
	OUT.pos   = mul(proj.projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas    = sfg_get_texture<Texture2D>(sfg_constant_mat0);
	float4    sampled  = atlas.SampleLevel(smp, IN.uv, 0);
	float3    coverage = sampled.rgb;
	float     alpha    = max(coverage.r, max(coverage.g, coverage.b));
	float3    weight   = coverage * IN.color.a;
	return float4(IN.color.rgb * weight, IN.color.a * alpha);
}
