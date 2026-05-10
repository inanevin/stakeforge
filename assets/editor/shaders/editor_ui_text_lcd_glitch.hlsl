#include "layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

SamplerState smp : static_sampler_nearest;

static const float GLITCH_TIME_SCALE		 = 1.0f;
static const float GLITCH_SCANLINE_SCALE	 = 0.45f;
static const float GLITCH_SCANLINE_MIN		 = 0.82f;
static const float GLITCH_SCANLINE_MAX		 = 1.0f;
static const float GLITCH_MAJOR_BAND_SCALE	 = 0.075f;
static const float GLITCH_MAJOR_BAND_SPEED	 = 7.0f;
static const float GLITCH_MAJOR_BAND_CUTOFF	 = 0.965f;
static const float GLITCH_MAJOR_JITTER_SCALE = 0.11f;
static const float GLITCH_MAJOR_JITTER_SPEED = 9.0f;
static const float GLITCH_MAJOR_UV_SHIFT	 = 0.0035f;
static const float GLITCH_MAJOR_TINT		 = 0.35f;
static const float GLITCH_MINOR_BAND_SCALE	 = 0.22f;
static const float GLITCH_MINOR_BAND_SPEED	 = 13.0f;
static const float GLITCH_MINOR_BAND_CUTOFF	 = 0.985f;
static const float GLITCH_MINOR_UV_SHIFT	 = 0.0015f;
static const float GLITCH_MINOR_TINT		 = 0.24f;
static const float GLITCH_STRIKE_SCALE		 = 0.11f;
static const float GLITCH_STRIKE_SPEED		 = 2.25f;
static const float GLITCH_STRIKE_START		 = 0.72f;
static const float GLITCH_STRIKE_END		 = 0.79f;
static const float GLITCH_STRIKE_INTENSITY	 = 0.35f;
static const float GLITCH_STRIKE_ALPHA		 = 0.22f;
static const float GLITCH_CHANNEL_SPLIT		 = 0.00014f;
static const float GLITCH_CHANNEL_SPLIT_PULSE = 0.00012f;
static const float GLITCH_CHANNEL_SPLIT_SPEED = 3.5f;
static const float GLITCH_BASE_ALPHA		 = 0.88f;
static const float GLITCH_MAJOR_ALPHA		 = 0.35f;

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
	Texture2D atlas						= sfg_get_texture<Texture2D>(sfg_constant_mat0);

	const float t			 = globals.sfg_global_elapsed * GLITCH_TIME_SCALE;
	const float scan		 = lerp(GLITCH_SCANLINE_MIN, GLITCH_SCANLINE_MAX, step(0.42f, frac(IN.pixel.y * GLITCH_SCANLINE_SCALE)));
	const float major_band	 = step(GLITCH_MAJOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_MAJOR_BAND_SCALE) + floor(t * GLITCH_MAJOR_BAND_SPEED) * 17.0f));
	const float minor_band	 = step(GLITCH_MINOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_MINOR_BAND_SCALE) + floor(t * GLITCH_MINOR_BAND_SPEED) * 31.0f));
	const float jitter		 = (hash11(floor(IN.pixel.y * GLITCH_MAJOR_JITTER_SCALE) + floor(t * GLITCH_MAJOR_JITTER_SPEED) * 23.0f) - 0.5f) * major_band;
	const float strike_phase = frac(IN.pixel.y * GLITCH_STRIKE_SCALE + t * GLITCH_STRIKE_SPEED);
	const float strike		 = step(GLITCH_STRIKE_START, strike_phase) * step(strike_phase, GLITCH_STRIKE_END);
	const float uv_shift		 = jitter * GLITCH_MAJOR_UV_SHIFT + minor_band * GLITCH_MINOR_UV_SHIFT;
	const float channel_split = GLITCH_CHANNEL_SPLIT + GLITCH_CHANNEL_SPLIT_PULSE * (0.5f + 0.5f * sin(t * GLITCH_CHANNEL_SPLIT_SPEED));

	const float r = atlas.SampleLevel(smp, IN.uv + float2(uv_shift + channel_split, 0.0f), 0).r;
	const float g = atlas.SampleLevel(smp, IN.uv + float2(uv_shift, 0.0f), 0).g;
	const float b = atlas.SampleLevel(smp, IN.uv + float2(uv_shift - channel_split, 0.0f), 0).b;

	float3 coverage = float3(r, g, b);
	float alpha		= max(coverage.r, max(coverage.g, coverage.b));
	float3 tint		= lerp(IN.color.rgb, float3(0.25f, 0.85f, 1.0f), major_band * GLITCH_MAJOR_TINT);
	tint			= lerp(tint, float3(1.0f, 0.15f, 0.65f), minor_band * GLITCH_MINOR_TINT);
	tint		   += strike * alpha * GLITCH_STRIKE_INTENSITY * float3(1.0f, 0.14f, 0.8f);

	float3 rgb = tint * coverage * IN.color.a * scan;
	alpha	   = saturate(alpha * IN.color.a * (GLITCH_BASE_ALPHA + major_band * GLITCH_MAJOR_ALPHA + strike * GLITCH_STRIKE_ALPHA));
	return float4(rgb, alpha);
}
