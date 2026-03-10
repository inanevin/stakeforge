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

#pragma once

#include "math_common.hpp"
#include "common/size_definitions.hpp"

#undef min
#undef max

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class istream;
	class ostream;

	struct vector2ui16;

	class vector2
	{
	public:
		vector2() {};
		vector2(f32 _x, f32 _y) : x(_x), y(_y) {};
		vector2(const vector2ui16& v);

		f32 x = 0.0f;
		f32 y = 0.0f;

		static vector2 zero;
		static vector2 one;

		static vector2 clamp(const vector2& vector, const vector2& min, const vector2& max);
		static vector2 clamp_magnitude(const vector2& vector, f32 max_length);
		static vector2 abs(const vector2& vector);
		static vector2 min(const vector2& a, const vector2& b);
		static vector2 max(const vector2& a, const vector2& b);
		static f32	   dot(const vector2& a, const vector2& b);
		static f32	   distance(const vector2& a, const vector2& b);
		static f32	   angle(const vector2& a, const vector2& b);

		vector2 normalized() const;
		bool	equals(const vector2& other, f32 epsilon = MATH_EPS) const;
		bool	is_zero(f32 epsilon = MATH_EPS) const;
		f32		magnitude() const;
		f32		magnitude_sqr() const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline vector2 operator+(const vector2& other) const
		{
			return vector2(x + other.x, y + other.y);
		}
		inline vector2 operator-(const vector2& other) const
		{
			return vector2(x - other.x, y - other.y);
		}
		inline vector2 operator*(f32 scalar) const
		{
			return vector2(x * scalar, y * scalar);
		}

		inline vector2 operator/(f32 scalar) const
		{
			if (scalar == 0.0f)
			{
				return vector2(std::numeric_limits<f32>::infinity(), std::numeric_limits<f32>::infinity());
			}
			return vector2(x / scalar, y / scalar);
		}

		inline vector2& operator+=(const vector2& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		inline vector2& operator-=(const vector2& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		inline vector2& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		inline vector2& operator/=(f32 scalar)
		{
			if (scalar == 0.0f)
			{
				x = MATH_INF_F;
				y = MATH_INF_F;
			}
			else
			{
				x /= scalar;
				y /= scalar;
			}
			return *this;
		}

		inline bool operator==(const vector2& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vector2& other) const
		{
			return !(*this == other);
		}
	};

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const vector2& v);
	void from_json(const nlohmann::json& j, vector2& v);

#endif
}
