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

#pragma once

#include "math_common.hpp"
#include <cmath>

#undef min
#undef max

namespace sfg
{
	namespace math
	{
		unsigned int floor_log2(unsigned int val);
		double		 fast_pow(double base, double exponent);

		template <typename T> inline bool is_nan(T val)
		{
			return std::isnan(val);
		}

		template <typename T> inline T copysign(T number, T sign)
		{
			return std::copysignf(number, sign);
		}

		template <typename T> inline T round(T val)
		{
			return std::round(val);
		}

		template <typename T> inline T lround(T val)
		{
			return std::lround(val);
		}

		template <typename T> inline T ceil(T val)
		{
			return std::ceil(val);
		}

		template <typename T> inline T floor(T val)
		{
			return std::floor(val);
		}

		template <typename T> inline T acos(T val)
		{
			return std::acos(val);
		}

		template <typename T> inline T clamp(T value, T min_val, T max_val)
		{
			return std::fmax(min_val, std::fmin(value, max_val));
		}
		template <typename T> inline T max(T a, T b)
		{
			return std::fmax(a, b);
		}
		template <typename T> inline T min(T a, T b)
		{
			return std::fmin(a, b);
		}
		template <typename T> inline T abs(T value)
		{
			return std::fabs(value);
		}
		template <typename T> inline T pow(T base, T exponent)
		{
			return std::pow(base, exponent);
		}
		template <typename T> inline T degrees_to_radians(T degrees)
		{
			return degrees * DEG_2_RAD;
		}
		template <typename T> inline T radians_to_degrees(T radians)
		{
			return radians * RAD_2_DEG;
		}
		template <typename T> inline bool almost_equal(T a, T b, T epsilon = MATH_EPS)
		{
			return abs(a - b) < epsilon;
		}
		template <typename T> inline int sign(T value)
		{
			return (T(0) < value) - (value < T(0));
		}
		template <typename T> inline T lerp(T a, T b, T t)
		{
			return a + t * (b - a);
		}
		template <typename T> inline T inverse_lerp(T a, T b, T value)
		{
			if (std::fabs(b - a) < MATH_EPS)
				return T(0);
			return (value - a) / (b - a);
		}

		template <typename T> inline T remap(T value, T in_min, T in_max, T out_min, T out_max)
		{
			if (std::fabs(in_max - in_min) < MATH_EPS)
				return out_min;
			T normalized_value = (value - in_min) / (in_max - in_min);
			return out_min + normalized_value * (out_max - out_min);
		}

		inline f32 fmodf(f32 value, f32 mod)
		{
			return std::fmodf(value, mod);
		}
		inline f32 modf(f32 value, f32* integral_part)
		{
			return std::modf(value, integral_part);
		}
		inline double modf(double value, double* integral_part)
		{
			return std::modf(value, integral_part);
		}
		inline f32 cos(f32 angle_rad)
		{
			return std::cos(angle_rad);
		}
		inline double cos(double angle_rad)
		{
			return std::cos(angle_rad);
		}
		inline f32 sin(f32 angle_rad)
		{
			return std::sin(angle_rad);
		}
		inline double sin(double angle_rad)
		{
			return std::sin(angle_rad);
		}
		inline f32 round(f32 value)
		{
			return std::round(value);
		}
		inline double round(double value)
		{
			return std::round(value);
		}
		inline f32 sqrt(f32 value)
		{
			return std::sqrtf(value);
		}
		inline f32 tan(f32 value)
		{
			return std::tanf(value);
		}
	}
}
