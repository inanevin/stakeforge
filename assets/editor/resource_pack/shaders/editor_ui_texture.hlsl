#include "../../../common/shaders/ui.hlsl"

SamplerState samp_linear : static_sampler_linear;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D source_texture = sfg_get_texture_non_uniform<Texture2D>(IN.constants.x);
	return source_texture.Sample(samp_linear, IN.uv) * IN.color;
}
