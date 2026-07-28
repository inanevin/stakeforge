#include "layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

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

float3 hsv_to_rgb(float3 hsv)
{
	float3 rgb = abs(frac(hsv.xxx + float3(0.0f, 2.0f / 3.0f, 1.0f / 3.0f)) * 6.0f - 3.0f);
	return hsv.z * lerp(float3(1.0f, 1.0f, 1.0f), saturate(rgb - 1.0f), hsv.y);
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	const float2 p = IN.uv * 2.0f - 1.0f;
	const float radius = length(p);
	const float edge = max(fwidth(radius), 0.001f);
	const float alpha = 1.0f - smoothstep(1.0f - edge, 1.0f + edge, radius);

	const float hue = frac(atan2(-p.y, p.x) / 6.28318530718f);
	float3 rgb = hsv_to_rgb(float3(hue, saturate(radius), 1.0f));
	rgb = lerp(rgb / 12.92f, pow((rgb + 0.055f) / 1.055f, 2.4f), step(0.04045f, rgb));
	return float4(rgb, alpha);
}
