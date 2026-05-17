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
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#undef min
#undef max


namespace sfg
{
	class istream_t;
	class ostream_t;

	struct vec4f_t
	{
		f32 x;
		f32 y;
		f32 z;
		f32 w;

		static const vec4f_t zero;
		static const vec4f_t one;

		static vec4f_t clamp(const vec4f_t& vector_t, const vec4f_t& min, const vec4f_t& max);
		static vec4f_t abs(const vec4f_t& vector);
		static vec4f_t min(const vec4f_t& a, const vec4f_t& b);
		static vec4f_t max(const vec4f_t& a, const vec4f_t& b);
		static f32	   dot(const vec4f_t& a, const vec4f_t& b);
		static f32	   distance(const vec4f_t& a, const vec4f_t& b);
		vec4f_t		   project(const vec4f_t& on_normal) const;
		vec4f_t		   rotate(const vec4f_t& axis, f32 angle_degrees) const;
		bool		   equals(const vec4f_t& other, f32 epsilon = MATH_EPS) const;
		bool		   is_zero(f32 epsilon = MATH_EPS) const;
		f32			   magnitude() const;
		f32			   magnitude_sqr() const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline bool is_point_inside(f32 _x, f32 _y) const
		{
			return _x >= x && _x <= x + z && _y >= y && _y <= y + w;
		}

		inline vec4f_t normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return {x / mag, y / mag, z / mag, w / mag};
			}
			return vec4f_t::zero;
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

		inline vec4f_t operator+(const vec4f_t& other) const
		{
			return {x + other.x, y + other.y, z + other.z, w + other.w};
		}
		inline vec4f_t operator-(const vec4f_t& other) const
		{
			return {x - other.x, y - other.y, z - other.z, w - other.w};
		}
		inline vec4f_t operator*(f32 scalar) const
		{
			return {x * scalar, y * scalar, z * scalar, w * scalar};
		}
		vec4f_t operator/(f32 scalar) const;

		inline vec4f_t operator-() const
		{
			return {-x, -y, -z, -w};
		}

		inline vec4f_t& operator+=(const vec4f_t& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}
		inline vec4f_t& operator-=(const vec4f_t& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}
		inline vec4f_t& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		vec4f_t& operator/=(f32 scalar);

		inline bool operator==(const vec4f_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vec4f_t& other) const
		{
			return !equals(other);
		}
	};

	inline vec4f_t operator*(f32 scalar, const vec4f_t& vector)
	{
		return vector * scalar;
	}

	void to_json(nlohmann::json& j, const vec4f_t& v);
	void from_json(const nlohmann::json& j, vec4f_t& v);

}
