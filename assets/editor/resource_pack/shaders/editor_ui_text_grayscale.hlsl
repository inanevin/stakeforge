#include "../../../common/shaders/ui.hlsl"

SamplerState smp : static_sampler_nearest;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas	  = sfg_get_texture_non_uniform<Texture2D>(IN.constants.x);
	float	  coverage = atlas.SampleLevel(smp, IN.uv, 0).r;
	return float4(IN.color.rgb, IN.color.a * coverage);
}
