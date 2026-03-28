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

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

#undef min
#undef max

namespace SFG
{
	class istream;
	class ostream;

	class vec3f
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;

		vec3f() = default;
		vec3f(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z)
		{
		}

		static const vec3f zero;
		static const vec3f one;
		static const vec3f up;
		static const vec3f forward;
		static const vec3f right;

		static vec3f clamp(const vec3f& vector, const vec3f& min, const vec3f& max);
		static vec3f cross(const vec3f& a, const vec3f& b);
		static vec3f abs(const vec3f& vector);
		static vec3f min(const vec3f& a, const vec3f& b);
		static vec3f max(const vec3f& a, const vec3f& b);
		static vec3f lerp(const vec3f& a, const vec3f& b, f32 t);
		static f32	 dot(const vec3f& a, const vec3f& b);
		static f32	 distance(const vec3f& a, const vec3f& b);
		static f32	 distance_sqr(const vec3f& a, const vec3f& b);
		vec3f		 project(const vec3f& on_normal) const;
		vec3f		 rotate(const vec3f& axis, f32 angle_degrees) const;
		vec3f		 reflect(const vec3f& in_normal) const;
		bool		 equals(const vec3f& other, f32 epsilon = MATH_EPS) const;
		bool		 is_zero(f32 epsilon = MATH_EPS) const;
		f32			 magnitude() const;
		f32			 magnitude_sqr() const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline vec3f normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return vec3f(x / mag, y / mag, z / mag);
			}
			return vec3f::zero;
		}

		inline void normalize()
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				x /= mag;
				y /= mag;
				z /= mag;
			}
			else
				x = y = z = 0.0f;
		}

		inline vec3f operator+(const vec3f& other) const
		{
			return vec3f(x + other.x, y + other.y, z + other.z);
		}
		inline vec3f operator-(const vec3f& other) const
		{
			return vec3f(x - other.x, y - other.y, z - other.z);
		}
		inline vec3f operator*(f32 scalar) const
		{
			return vec3f(x * scalar, y * scalar, z * scalar);
		}
		inline vec3f operator/(f32 scalar) const
		{
			if (scalar == 0.0f)
				return vec3f::zero;
			return vec3f(x / scalar, y / scalar, z / scalar);
		}

		inline vec3f operator*(const vec3f& other) const
		{
			return vec3f(x * other.x, y * other.y, z * other.z);
		}

		inline vec3f operator-() const
		{
			return vec3f(-x, -y, -z);
		}

		inline vec3f& operator+=(const vec3f& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}
		inline vec3f& operator-=(const vec3f& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}
		inline vec3f& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		inline vec3f& operator/=(f32 scalar)
		{
			if (scalar != 0.0f)
			{
				x /= scalar;
				y /= scalar;
				z /= scalar;
			}
			else
			{
				x = MATH_INF_F;
				y = MATH_INF_F;
				z = MATH_INF_F;
			}
			return *this;
		}

		inline bool operator==(const vec3f& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vec3f& other) const
		{
			return !equals(other);
		}
	};

	inline vec3f operator*(f32 scalar, const vec3f& vector)
	{
		return vector * scalar;
	}

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const vec3f& v);
	void from_json(const nlohmann::json& j, vec3f& v);

#endif
}
