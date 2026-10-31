#include "../../../common/shaders/ui.hlsl"

SamplerState samp_nearest : static_sampler_nearest;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D source_texture = sfg_get_texture_non_uniform<Texture2D>(IN.constants.x);
	uint mip_level			 = IN.constants.y;
	return source_texture.SampleLevel(samp_nearest, IN.uv, mip_level);
}
