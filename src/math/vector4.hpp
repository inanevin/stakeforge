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

#undef min
#undef max

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{

	class istream;
	class ostream;

	class vector4
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 0.0f;

		vector4() = default;
		vector4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w)
		{
		}

		static const vector4 zero;
		static const vector4 one;

		static vector4 clamp(const vector4& vector, const vector4& min, const vector4& max);
		static vector4 abs(const vector4& vector);
		static vector4 min(const vector4& a, const vector4& b);
		static vector4 max(const vector4& a, const vector4& b);
		static f32	   dot(const vector4& a, const vector4& b);
		static f32	   distance(const vector4& a, const vector4& b);
		vector4		   project(const vector4& on_normal) const;
		vector4		   rotate(const vector4& axis, f32 angle_degrees) const;
		bool		   equals(const vector4& other, f32 epsilon = MATH_EPS) const;
		bool		   is_zero(f32 epsilon = MATH_EPS) const;
		f32			   magnitude() const;
		f32			   magnitude_sqr() const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline bool is_point_inside(f32 _x, f32 _y) const
		{
			return _x >= x && _x <= x + z && _y >= y && _y <= y + w;
		}

		inline vector4 normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return vector4(x / mag, y / mag, z / mag, w / mag);
			}
			return vector4::zero;
		}

		inline void normalize()
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				x /= mag;
				y /= mag;
				z /= mag;
				w /= mag;
			}
			else
			{
				x = y = z = w = 0.0f;
			}
		}

		inline vector4 operator+(const vector4& other) const
		{
			return vector4(x + other.x, y + other.y, z + other.z, w + other.w);
		}
		inline vector4 operator-(const vector4& other) const
		{
			return vector4(x - other.x, y - other.y, z - other.z, w - other.w);
		}
		inline vector4 operator*(f32 scalar) const
		{
			return vector4(x * scalar, y * scalar, z * scalar, w * scalar);
		}
		vector4 operator/(f32 scalar) const;

		inline vector4 operator-() const
		{
			return vector4(-x, -y, -z, -w);
		}

		inline vector4& operator+=(const vector4& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}
		inline vector4& operator-=(const vector4& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}
		inline vector4& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		vector4& operator/=(f32 scalar);

		inline bool operator==(const vector4& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vector4& other) const
		{
			return !equals(other);
		}
	};

	inline vector4 operator*(f32 scalar, const vector4& vector)
	{
		return vector * scalar;
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const vector4& v);
	void from_json(const nlohmann::json& j, vector4& v);

#endif

}