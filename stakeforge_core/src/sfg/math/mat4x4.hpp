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
#include "vec4f.hpp"

namespace sfg
{
	class quat_t;

	class ostream_t;
	class istream_t;

	class mat4x4_t
	{
	public:
		f32 m[16]; // Column-major storage: m[col * 4 + row]

		mat4x4_t() = default;
		mat4x4_t(f32 m00,
				 f32 m10,
				 f32 m20,
				 f32 m30, // Col 0
				 f32 m01,
				 f32 m11,
				 f32 m21,
				 f32 m31, // Col 1
				 f32 m02,
				 f32 m12,
				 f32 m22,
				 f32 m32, // Col 2
				 f32 m03,
				 f32 m13,
				 f32 m23,
				 f32 m33); // Col 3

		static const mat4x4_t identity;

		mat4x4_t get_normal_matrix() const;
		mat4x4_t transpose() const;
		f32		 determinant() const;
		mat4x4_t inverse() const;
		vec3f_t	 get_scale() const;
		vec3f_t	 get_translation() const;
		bool	 equals(const mat4x4_t& other, f32 epsilon = MATH_EPS) const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		static mat4x4_t translation(const vec3f_t& t);
		static mat4x4_t scale(const vec3f_t& s);
		static mat4x4_t rotation(const quat_t& q);
		static mat4x4_t ortho_reverse_z(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);
		static mat4x4_t ortho(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);
		static mat4x4_t perspective_reverse_z(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane);
		static mat4x4_t perspective(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane);
		static mat4x4_t transform(const vec3f_t& position, const quat_t& rotation, const vec3f_t& scale);
		static mat4x4_t look_at(const vec3f_t& eye, const vec3f_t& target, const vec3f_t& up);
		static mat4x4_t view(const quat_t& rot, const vec3f_t& pos);

		inline f32 operator[](int index) const
		{
			return m[index];
		}
		inline f32& operator[](int index)
		{
			return m[index];
		}

		inline bool operator==(const mat4x4_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const mat4x4_t& other) const
		{
			return !equals(other);
		}

		inline mat4x4_t operator*(const mat4x4_t& other) const
		{
			mat4x4_t result;
			for (int i = 0; i < 4; ++i) // Result columns
			{
				for (int j = 0; j < 4; ++j) // Result rows
				{
					result.m[i * 4 + j] = m[0 * 4 + j] * other.m[i * 4 + 0] + m[1 * 4 + j] * other.m[i * 4 + 1] + m[2 * 4 + j] * other.m[i * 4 + 2] + m[3 * 4 + j] * other.m[i * 4 + 3];
				}
			}
			return result;
		}

		inline vec4f_t operator*(const vec4f_t& v) const
		{
			return vec4f_t(m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w, m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w, m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w, m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w);
		}

		vec3f_t operator*(const vec3f_t& v) const;

		inline mat4x4_t operator*(f32 scalar) const
		{
			mat4x4_t result;
			for (int i = 0; i < 16; ++i)
			{
				result.m[i] = m[i] * scalar;
			}
			return result;
		}

		mat4x4_t operator/(f32 scalar) const;

		inline mat4x4_t& operator*=(const mat4x4_t& other)
		{
			*this = (*this) * other;
			return *this;
		}

		inline mat4x4_t& operator*=(f32 scalar)
		{
			for (int i = 0; i < 16; ++i)
			{
				m[i] *= scalar;
			}
			return *this;
		}

		mat4x4_t& operator/=(f32 scalar);
	};

	inline mat4x4_t operator*(f32 scalar, const mat4x4_t& mat)
	{
		return mat * scalar;
	}

	SFG_DEFINE_TYPE_ID(mat4x4_t);

	struct mat4x4_reflection_t
	{
		mat4x4_reflection_t();
	};

	inline mat4x4_reflection_t g_reflect_mat4x4;
}
