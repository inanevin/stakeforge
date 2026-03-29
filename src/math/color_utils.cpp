/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "common/size_definitions.hpp"
#include "color_utils.hpp"
#include "color.hpp"
#include "math.hpp"
#include "easing.hpp"
#include <iomanip>
#include <sstream>

namespace SFG
{
	color color_utils::lerp(const color& c1, const color& c2, f32 a)
	{
		return color(easing::lerp(c1.x, c2.x, a), easing::lerp(c1.y, c2.y, a), easing::lerp(c1.z, c2.z, a), easing::lerp(c1.w, c2.w, a));
	}

	color color_utils::from_hex(const string_t& hex)
	{
		if (hex.size() != 7)
			return color::black;

		if (hex[0] != '#')
			return color::black;

		u32 r, g, b;

#ifdef SFG_PLATFORM_WINDOWS
		sscanf_s(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
#else
		const int ret = std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
#endif
		return color{static_cast<f32>(r) / 255.0f, static_cast<f32>(g) / 255.0f, static_cast<f32>(b) / 255.0f, 1.0f};
	}

	string_t color_utils::to_hex(const color& color)
	{
		const i32		  r = static_cast<i32>(color.x * 255);
		const i32		  g = static_cast<i32>(color.y * 255);
		const i32		  b = static_cast<i32>(color.z * 255);
		std::stringstream ss;
		ss << "#" << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
		return ss.str();
	}

	color color_utils::hs_to_srgb(const color& col)
	{
		const f32 hue		 = col.x;
		const f32 saturation = col.y;
		const f32 angle		 = hue * 6.0f;
		const f32 r			 = math::clamp(math::abs(angle - 3.0f) - 1.0f, 0.0f, 1.0f);
		const f32 g			 = math::clamp(2.0f - math::abs(angle - 2.0f), 0.0f, 1.0f);
		const f32 b			 = math::clamp(2.0f - math::abs(angle - 4.0f), 0.0f, 1.0f);
		return lerp(color::white, color(r, g, b, 1.0f), saturation);
	}

	color color_utils::srgb_to_hsv(const color& col)
	{
		color hsv;

		f32 minVal = math::min(col.x, math::min(col.y, col.z));
		f32 maxVal = math::max(col.x, math::max(col.y, col.z));
		f32 delta  = maxVal - minVal;

		// Hue calculation
		if (delta == 0)
		{
			hsv.x = 0;
		}
		else if (maxVal == col.x)
		{
			f32 integ = 0.0f;
			hsv.x	  = 60 * math::modf(((col.y - col.z) / delta), &integ);
		}
		else if (maxVal == col.y)
		{
			hsv.x = 60 * (((col.z - col.x) / delta) + 2);
		}
		else if (maxVal == col.z)
		{
			hsv.x = 60 * (((col.x - col.y) / delta) + 4);
		}

		if (hsv.x < 0)
		{
			hsv.x += 360;
		}

		// Saturation calculation
		hsv.y = (maxVal == 0) ? 0 : (delta / maxVal);

		// Value calculation
		hsv.z = maxVal;
		hsv.w = col.w;

		return hsv;
	}

	color color_utils::hsv_to_srgb(const color& col)
	{
		color rgb;
		f32	  C		= col.z * col.y;
		f32	  integ = 0.0f;
		f32	  X		= C * (1 - math::abs(math::modf(col.x / 60.0f, &integ) - 1.0f));
		f32	  m		= col.z - C;
		f32	  R1, G1, B1;

		if (col.x >= 0 && col.x < 60)
		{
			R1 = C;
			G1 = X;
			B1 = 0;
		}
		else if (col.x >= 60 && col.x < 120)
		{
			R1 = X;
			G1 = C;
			B1 = 0;
		}
		else if (col.x >= 120 && col.x < 180)
		{
			R1 = 0;
			G1 = C;
			B1 = X;
		}
		else if (col.x >= 180 && col.x < 240)
		{
			R1 = 0;
			G1 = X;
			B1 = C;
		}
		else if (col.x >= 240 && col.x < 300)
		{
			R1 = X;
			G1 = 0;
			B1 = C;
		}
		else
		{
			R1 = C;
			G1 = 0;
			B1 = X;
		}

		rgb.x = R1 + m;
		rgb.y = G1 + m;
		rgb.z = B1 + m;
		rgb.w = col.w;

		return rgb;
	}

	color color_utils::srgb_to_linear(const color& col)
	{
		auto convert = [](f32 value) {
			if (value <= 0.04045f)
			{
				return value / 12.92f;
			}
			else
			{
				return math::pow((value + 0.055f) / 1.055f, 2.4f);
			}
		};

		return color(convert(col.x), convert(col.y), convert(col.z), convert(col.w));
	}

	color color_utils::linear_to_srgb(const color& col)
	{
		auto convert = [](f32 value) {
			if (value <= 0.0031308f)
			{
				return value * 12.92f;
			}
			else
			{
				return 1.055f * math::pow(value, 1.0f / 2.4f) - 0.055f;
			}
		};

		return color(convert(col.x), convert(col.y), convert(col.z), convert(col.w));
	}

	color color_utils::brighten(const color& color, f32 amt)
	{
		return lerp(color, color::white, amt);
	}

	color color_utils::darken(const color& color, f32 amt)
	{
		return lerp(color, color::white, amt);
	}

} // namespace SFG
