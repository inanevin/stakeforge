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

#include "vec3f.hpp"

namespace sfg
{
	class ostrem;
	class istream_t;

	class quat_t
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 1.0f;

		quat_t() = default;
		quat_t(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w)
		{
		}

		static const quat_t identity;

		vec3f_t get_right() const;
		vec3f_t get_up() const;
		vec3f_t get_forward() const;
		quat_t	conjugate() const;
		quat_t	inverse() const;
		quat_t	normalized() const;
		f32		dot(const quat_t& other) const;
		f32		magnitude() const;
		f32		sqr_magnitude() const;
		void	normalize();
		bool	equals(const quat_t& other, f32 epsilon = MATH_EPS) const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		static quat_t  from_euler(f32 pitch_degrees, f32 yaw_degrees, f32 roll_degrees);
		static vec3f_t to_euler(const quat_t& q);
		static quat_t  angle_axis(f32 angle_degrees, const vec3f_t& axis);
		static quat_t  lerp(const quat_t& a, const quat_t& b, f32 t);
		static quat_t  slerp(const quat_t& a, const quat_t& b, f32 t);
		static quat_t  look_at(const vec3f_t& source_point, const vec3f_t& target_point, const vec3f_t& up_vector);
		static quat_t  from_rotation_matrix3x3(const f32 R_m[9]);

		inline bool is_identity(f32 epsilon = MATH_EPS) const
		{
			return equals(identity, epsilon);
		}

		inline quat_t operator+(const quat_t& other) const
		{
			return quat_t(x + other.x, y + other.y, z + other.z, w + other.w);
		}

		inline quat_t operator-(const quat_t& other) const
		{
			return quat_t(x - other.x, y - other.y, z - other.z, w - other.w);
		}

		inline quat_t operator*(const quat_t& other) const
		{
			return quat_t(w * other.x + x * other.w + y * other.z - z * other.y, w * other.y + y * other.w + z * other.x - x * other.z, w * other.z + z * other.w + x * other.y - y * other.x, w * other.w - x * other.x - y * other.y - z * other.z);
		}

		inline vec3f_t operator*(const vec3f_t& v) const
		{
			quat_t p(v.x, v.y, v.z, 0.0f);
			quat_t q_inv	 = this->conjugate();
			quat_t rotated_p = (*this) * p * q_inv;
			return vec3f_t(rotated_p.x, rotated_p.y, rotated_p.z);
		}

		inline quat_t operator*(f32 scalar) const
		{
			return quat_t(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		quat_t operator/(f32 scalar) const;

		inline quat_t operator-() const
		{
			return quat_t(-x, -y, -z, -w);
		}

		inline quat_t& operator*=(const quat_t& other)
		{
			*this = (*this) * other;
			return *this;
		}

		inline quat_t& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		quat_t& operator/=(f32 scalar);

		inline bool operator==(const quat_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const quat_t& other) const
		{
			return !equals(other);
		}
	};

	inline quat_t operator*(f32 scalar, const quat_t& q)
	{
		return q * scalar;
	}

	SFG_DEFINE_TYPE_ID(quat_t);

	struct quat_reflection_t
	{
		quat_reflection_t();
	};

	inline quat_reflection_t g_reflect_quat;
}
