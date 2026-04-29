#define UI_TEXT_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
	"RootConstants(num32BitConstants=16, b0, space=0, visibility=SHADER_VISIBILITY_VERTEX)," \
	"RootConstants(num32BitConstants=4, b1, space=0, visibility=SHADER_VISIBILITY_PIXEL)," \
	"StaticSampler(s0, " \
		"filter=FILTER_MIN_MAG_MIP_LINEAR, " \
		"addressU=TEXTURE_ADDRESS_CLAMP, " \
		"addressV=TEXTURE_ADDRESS_CLAMP, " \
		"addressW=TEXTURE_ADDRESS_CLAMP, " \
		"borderColor=STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
		"visibility=SHADER_VISIBILITY_PIXEL)"

cbuffer projection_cb : register(b0, space0)
{
	float4x4 projection;
};

cbuffer mat_cb : register(b1, space0)
{
	uint atlas_idx;
	uint _pad0;
	uint _pad1;
	uint _pad2;
};

SamplerState samp_linear : register(s0, space0);

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

[RootSignature(UI_TEXT_RS)]
VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	OUT.pos   = mul(projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas    = ResourceDescriptorHeap[atlas_idx];
	float     coverage = atlas.SampleLevel(samp_linear, IN.uv, 0).r;
	return float4(IN.color.rgb, IN.color.a * coverage);
}
