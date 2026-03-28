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

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class istream;
	class ostream;

	class vec4f
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 0.0f;

		vec4f() = default;
		vec4f(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w)
		{
		}

		static const vec4f zero;
		static const vec4f one;

		static vec4f clamp(const vec4f& vector, const vec4f& min, const vec4f& max);
		static vec4f abs(const vec4f& vector);
		static vec4f min(const vec4f& a, const vec4f& b);
		static vec4f max(const vec4f& a, const vec4f& b);
		static f32	 dot(const vec4f& a, const vec4f& b);
		static f32	 distance(const vec4f& a, const vec4f& b);
		vec4f		 project(const vec4f& on_normal) const;
		vec4f		 rotate(const vec4f& axis, f32 angle_degrees) const;
		bool		 equals(const vec4f& other, f32 epsilon = MATH_EPS) const;
		bool		 is_zero(f32 epsilon = MATH_EPS) const;
		f32			 magnitude() const;
		f32			 magnitude_sqr() const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline bool is_point_inside(f32 _x, f32 _y) const
		{
			return _x >= x && _x <= x + z && _y >= y && _y <= y + w;
		}

		inline vec4f normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return vec4f(x / mag, y / mag, z / mag, w / mag);
			}
			return vec4f::zero;
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

		inline vec4f operator+(const vec4f& other) const
		{
			return vec4f(x + other.x, y + other.y, z + other.z, w + other.w);
		}
		inline vec4f operator-(const vec4f& other) const
		{
			return vec4f(x - other.x, y - other.y, z - other.z, w - other.w);
		}
		inline vec4f operator*(f32 scalar) const
		{
			return vec4f(x * scalar, y * scalar, z * scalar, w * scalar);
		}
		vec4f operator/(f32 scalar) const;

		inline vec4f operator-() const
		{
			return vec4f(-x, -y, -z, -w);
		}

		inline vec4f& operator+=(const vec4f& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}
		inline vec4f& operator-=(const vec4f& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}
		inline vec4f& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		vec4f& operator/=(f32 scalar);

		inline bool operator==(const vec4f& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vec4f& other) const
		{
			return !equals(other);
		}
	};

	inline vec4f operator*(f32 scalar, const vec4f& vector)
	{
		return vector * scalar;
	}

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const vec4f& v);
	void from_json(const nlohmann::json& j, vec4f& v);

#endif

}
