// vekt bitmap-font text shader. Atlas is R8; .r is the coverage alpha.

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
