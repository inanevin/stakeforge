/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "canvas/ui.hlsl"

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

float hash11(float v)
{
	return frac(sin(v * 91.3458f) * 47453.5453f);
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	ConstantBuffer<sfg_globals> globals = sfg_get_cbv<sfg_globals>(sfg_constant_global0);
	Texture2D atlas						= sfg_get_texture_non_uniform<Texture2D>(IN.constants.x);

	const float t			 = globals.sfg_global_elapsed * GLITCH_TIME_SCALE;
	const float scan		 = lerp(GLITCH_SCANLINE_MIN, GLITCH_SCANLINE_MAX, step(0.42f, frac(IN.pixel.y * GLITCH_SCANLINE_SCALE)));
	const float major_band	 = step(GLITCH_MAJOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_MAJOR_BAND_SCALE) + floor(t * GLITCH_MAJOR_BAND_SPEED) * 17.0f));
	const float minor_band	 = step(GLITCH_MINOR_BAND_CUTOFF, hash11(floor(IN.pixel.y * GLITCH_MINOR_BAND_SCALE) + floor(t * GLITCH_MINOR_BAND_SPEED) * 31.0f));
	const float jitter		 = (hash11(floor(IN.pixel.y * GLITCH_MAJOR_JITTER_SCALE) + floor(t * GLITCH_MAJOR_JITTER_SPEED) * 23.0f) - 0.5f) * major_band;
	const float strike_phase = frac(IN.pixel.y * GLITCH_STRIKE_SCALE + t * GLITCH_STRIKE_SPEED);
	const float strike		 = step(GLITCH_STRIKE_START, strike_phase) * step(strike_phase, GLITCH_STRIKE_END);
	const float uv_shift		 = jitter * GLITCH_MAJOR_UV_SHIFT + minor_band * GLITCH_MINOR_UV_SHIFT;
	const float channel_split = GLITCH_CHANNEL_SPLIT + GLITCH_CHANNEL_SPLIT_PULSE * (0.5f + 0.5f * sin(t * GLITCH_CHANNEL_SPLIT_SPEED));

	const float r = ui_text_coverage(atlas, IN.uv + float2(uv_shift + channel_split, 0.0f)).r;
	const float g = ui_text_coverage(atlas, IN.uv + float2(uv_shift, 0.0f)).g;
	const float b = ui_text_coverage(atlas, IN.uv + float2(uv_shift - channel_split, 0.0f)).b;

	float3 coverage = float3(r, g, b);
	float alpha		= max(coverage.r, max(coverage.g, coverage.b));
	float3 tint		= lerp(IN.color.rgb, float3(0.25f, 0.85f, 1.0f), major_band * GLITCH_MAJOR_TINT);
	tint			= lerp(tint, float3(1.0f, 0.15f, 0.65f), minor_band * GLITCH_MINOR_TINT);
	tint		   += strike * alpha * GLITCH_STRIKE_INTENSITY * float3(1.0f, 0.14f, 0.8f);

	float3 weight = coverage * IN.color.a;
	float3 rgb	  = tint * weight * scan;
	alpha		  = saturate(alpha * IN.color.a * (GLITCH_BASE_ALPHA + major_band * GLITCH_MAJOR_ALPHA + strike * GLITCH_STRIKE_ALPHA));
#if defined(UI_TEXT_LCD)
	return float4(rgb, alpha);
#else
	return float4(tint * scan, alpha);
#endif
}
