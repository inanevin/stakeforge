#include "../../../common/shaders/ui.hlsl"

float4 PSMain(VSOutput IN) : SV_TARGET
{
	return IN.color;
}
