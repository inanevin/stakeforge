// vekt default geometry shader (rects, lines, AA bands).
// Root signature (built in C++):
//   b0, space0 : 16x32-bit constants (projection matrix), all stages
//   b1, space0 : 4x32-bit constants (mat_constants: atlas_idx, _, _, _), pixel stage
//   s0, space0 : immutable linear-clamp sampler

cbuffer projection_cb : register(b0, space0)
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
	return IN.color;
}
