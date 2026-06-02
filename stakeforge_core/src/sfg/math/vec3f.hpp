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
#include <sfg/common/size_definitions.hpp>

#undef min
#undef max

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct vec3f_t
	{
		f32 x;
		f32 y;
		f32 z;

		static const vec3f_t zero;
		static const vec3f_t one;
		static const vec3f_t up;
		static const vec3f_t forward;
		static const vec3f_t right;

		static vec3f_t clamp(const vec3f_t& vector_t, const vec3f_t& min, const vec3f_t& max);
		static vec3f_t cross(const vec3f_t& a, const vec3f_t& b);
		static vec3f_t abs(const vec3f_t& vector_t);
		static vec3f_t min(const vec3f_t& a, const vec3f_t& b);
		static vec3f_t max(const vec3f_t& a, const vec3f_t& b);
		static vec3f_t lerp(const vec3f_t& a, const vec3f_t& b, f32 t);
		static f32	   dot(const vec3f_t& a, const vec3f_t& b);
		static f32	   distance(const vec3f_t& a, const vec3f_t& b);
		static f32	   distance_sqr(const vec3f_t& a, const vec3f_t& b);
		vec3f_t		   project(const vec3f_t& on_normal) const;
		vec3f_t		   rotate(const vec3f_t& axis, f32 angle_degrees) const;
		vec3f_t		   reflect(const vec3f_t& in_normal) const;
		bool		   equals(const vec3f_t& other, f32 epsilon = MATH_EPS) const;
		bool		   is_zero(f32 epsilon = MATH_EPS) const;
		f32			   magnitude() const;
		f32			   magnitude_sqr() const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline vec3f_t normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return {x / mag, y / mag, z / mag};
			}
			return vec3f_t::zero;
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

		inline vec3f_t operator+(const vec3f_t& other) const
		{
			return {x + other.x, y + other.y, z + other.z};
		}
		inline vec3f_t operator-(const vec3f_t& other) const
		{
			return {x - other.x, y - other.y, z - other.z};
		}
		inline vec3f_t operator*(f32 scalar) const
		{
			return {x * scalar, y * scalar, z * scalar};
		}
		inline vec3f_t operator/(f32 scalar) const
		{
			if (scalar == 0.0f)
				return vec3f_t::zero;
			return {x / scalar, y / scalar, z / scalar};
		}

		inline vec3f_t operator*(const vec3f_t& other) const
		{
			return {x * other.x, y * other.y, z * other.z};
		}

		inline vec3f_t operator-() const
		{
			return {-x, -y, -z};
		}

		inline vec3f_t& operator+=(const vec3f_t& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}
		inline vec3f_t& operator-=(const vec3f_t& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}
		inline vec3f_t& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		inline vec3f_t& operator/=(f32 scalar)
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

		inline bool operator==(const vec3f_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vec3f_t& other) const
		{
			return !equals(other);
		}
	};

	inline vec3f_t operator*(f32 scalar, const vec3f_t& vector_t)
	{
		return vector_t * scalar;
	}

}
