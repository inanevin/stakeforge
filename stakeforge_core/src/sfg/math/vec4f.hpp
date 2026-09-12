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

#include <sfg/common/type_id.hpp>

#include "math_common.hpp"

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

	SFG_DEFINE_TYPE_ID(vec4f_t);

	struct vec4f_reflection_t
	{
		vec4f_reflection_t();
	};

	inline vec4f_reflection_t g_reflect_vec4f;
}
