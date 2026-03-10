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

#include "vector3.hpp"
#include "vector4.hpp"

namespace SFG
{
	class quat;

	class ostream;
	class istream;

	class matrix4x4
	{
	public:
		f32 m[16]; // Column-major storage: m[col * 4 + row]

		matrix4x4() = default;
		matrix4x4(f32 m00,
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

		static const matrix4x4 identity;

		matrix4x4 get_normal_matrix() const;
		matrix4x4 transpose() const;
		f32		  determinant() const;
		matrix4x4 inverse() const;
		vector3	  get_scale() const;
		vector3	  get_translation() const;
		bool	  equals(const matrix4x4& other, f32 epsilon = MATH_EPS) const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		static matrix4x4 translation(const vector3& t);
		static matrix4x4 scale(const vector3& s);
		static matrix4x4 rotation(const quat& q);
		static matrix4x4 ortho_reverse_z(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);
		static matrix4x4 ortho(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);
		static matrix4x4 perspective_reverse_z(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane);
		static matrix4x4 perspective(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane);
		static matrix4x4 transform(const vector3& position, const quat& rotation, const vector3& scale);
		static matrix4x4 look_at(const vector3& eye, const vector3& target, const vector3& up);
		static matrix4x4 view(const quat& rot, const vector3& pos);

		inline f32 operator[](int index) const
		{
			return m[index];
		}
		inline f32& operator[](int index)
		{
			return m[index];
		}

		inline bool operator==(const matrix4x4& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const matrix4x4& other) const
		{
			return !equals(other);
		}

		inline matrix4x4 operator*(const matrix4x4& other) const
		{
			matrix4x4 result;
			for (int i = 0; i < 4; ++i) // Result columns
			{
				for (int j = 0; j < 4; ++j) // Result rows
				{
					result.m[i * 4 + j] = m[0 * 4 + j] * other.m[i * 4 + 0] + m[1 * 4 + j] * other.m[i * 4 + 1] + m[2 * 4 + j] * other.m[i * 4 + 2] + m[3 * 4 + j] * other.m[i * 4 + 3];
				}
			}
			return result;
		}

		inline vector4 operator*(const vector4& v) const
		{
			return vector4(m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w, m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w, m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w, m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w);
		}

		vector3 operator*(const vector3& v) const;

		inline matrix4x4 operator*(f32 scalar) const
		{
			matrix4x4 result;
			for (int i = 0; i < 16; ++i)
			{
				result.m[i] = m[i] * scalar;
			}
			return result;
		}

		matrix4x4 operator/(f32 scalar) const;

		inline matrix4x4& operator*=(const matrix4x4& other)
		{
			*this = (*this) * other;
			return *this;
		}

		inline matrix4x4& operator*=(f32 scalar)
		{
			for (int i = 0; i < 16; ++i)
			{
				m[i] *= scalar;
			}
			return *this;
		}

		matrix4x4& operator/=(f32 scalar);
	};

	inline matrix4x4 operator*(f32 scalar, const matrix4x4& mat)
	{
		return mat * scalar;
	}
}
