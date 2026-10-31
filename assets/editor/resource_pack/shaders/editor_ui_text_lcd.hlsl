#include "../../../common/shaders/ui.hlsl"

SamplerState smp : static_sampler_nearest;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas    = sfg_get_texture_non_uniform<Texture2D>(IN.constants.x);
	float4    sampled  = atlas.SampleLevel(smp, IN.uv, 0);
	float3    coverage = pow(sampled.rgb, 1.0f / 1.43f);
	float     alpha    = max(coverage.r, max(coverage.g, coverage.b));
	float3    weight   = coverage * IN.color.a;
	return float4(IN.color.rgb * weight, IN.color.a * alpha);
}
