#include "layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

static const float GLITCH_RECT_SCANLINE_SCALE		= 0.48f;
static const float GLITCH_RECT_SCANLINE_MIN			= 0.72f;
static const float GLITCH_RECT_SCANLINE_MAX			= 1.08f;
static const float GLITCH_RECT_MINOR_BAND_SCALE		= 0.20f;
static const float GLITCH_RECT_MINOR_BAND_SPEED		= 16.0f;
static const float GLITCH_RECT_MINOR_BAND_CUTOFF	= 0.972f;
static const float GLITCH_RECT_MAJOR_BAND_SCALE		= 0.065f;
static const float GLITCH_RECT_MAJOR_BAND_SPEED		= 8.0f;
static const float GLITCH_RECT_MAJOR_BAND_CUTOFF	= 0.982f;
static const float GLITCH_RECT_TEAR_SCALE			= 0.11f;
static const float GLITCH_RECT_TEAR_SPEED			= 2.7f;
static const float GLITCH_RECT_TEAR_START			= 0.76f;
static const float GLITCH_RECT_TEAR_END				= 0.82f;
static const float GLITCH_RECT_EDGE_BOOST			= 0.20f;
static const float GLITCH_RECT_MAJOR_ALPHA_BOOST	= 0.32f;
static const float GLITCH_RECT_MINOR_ALPHA_BOOST	= 0.18f;
static const float GLITCH_RECT_TEAR_ALPHA_BOOST		= 0.22f;
static const float GLITCH_RECT_MAJOR_ALPHA_DROP		= 0.28f;
static const float GLITCH_RECT_CHANNEL_SPLIT_SPEED	= 4.2f;

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
	float2 pixel : TEXCOORD1;
};

float hash11(float v)
{
	return frac(sin(v * 91.3458f) * 47453.5453f);
}

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	ConstantBuffer<projection_cb> proj = sfg_get_cbv<projection_cb>(sfg_constant_rp0);
	OUT.pos							 = mul(proj.projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv							 = IN.uv;
	OUT.color						 = IN.color;
	OUT.pixel						 = IN.pos;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	ConstantBuffer<sfg_globals> globals = sfg_get_cbv<sfg_globals>(sfg_constant_global0);

	const float t			 = globals.sfg_global_elapsed;
	const float scan		 = lerp(GLITCH_RECT_SCANLINE_MIN, GLITCH_RECT_SCANLINE_MAX, step(0.5f, frac(IN.pixel.y * GLITCH_RECT_SCANLINE_SCALE)));
	const float minor_band	 = step(GLITCH_RECT_MINOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_RECT_MINOR_BAND_SCALE) + floor(t * GLITCH_RECT_MINOR_BAND_SPEED) * 31.0f));
	const float major_band	 = step(GLITCH_RECT_MAJOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_RECT_MAJOR_BAND_SCALE) + floor(t * GLITCH_RECT_MAJOR_BAND_SPEED) * 17.0f));
	const float tear_phase	 = frac(IN.pixel.y * GLITCH_RECT_TEAR_SCALE + t * GLITCH_RECT_TEAR_SPEED);
	const float tear		 = step(GLITCH_RECT_TEAR_START, tear_phase) * step(tear_phase, GLITCH_RECT_TEAR_END);
	const float edge_x		 = 1.0f - smoothstep(0.0f, 0.18f, min(IN.uv.x, 1.0f - IN.uv.x));
	const float edge_y		 = 1.0f - smoothstep(0.0f, 0.18f, min(IN.uv.y, 1.0f - IN.uv.y));
	const float edge		 = saturate(max(edge_x, edge_y));
	const float split		 = 0.5f + 0.5f * sin(t * GLITCH_RECT_CHANNEL_SPLIT_SPEED + IN.pixel.y * 0.11f);

	float3 color = IN.color.rgb * scan;
	color		 = lerp(color, float3(0.12f, 0.78f, 1.0f), edge * GLITCH_RECT_EDGE_BOOST);
	color		 = lerp(color, float3(1.0f, 0.14f, 0.65f), minor_band * (0.18f + split * 0.12f));
	color	   += major_band * float3(0.12f, 0.72f, 1.0f) * 0.26f;
	color	   += tear * float3(1.0f, 0.20f, 0.75f) * 0.20f;

	float alpha = IN.color.a;
	alpha		= saturate(alpha * (0.9f + edge * GLITCH_RECT_EDGE_BOOST + minor_band * GLITCH_RECT_MINOR_ALPHA_BOOST + major_band * GLITCH_RECT_MAJOR_ALPHA_BOOST + tear * GLITCH_RECT_TEAR_ALPHA_BOOST));
	alpha	   *= 1.0f - major_band * GLITCH_RECT_MAJOR_ALPHA_DROP * step(0.55f, hash11(floor(t * 24.0f) + floor(IN.pixel.y * 0.18f)));

	return float4(color, alpha);
}
