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

#include "common/size_definitions.hpp"
#include "vector4.hpp"
#include "vector3.hpp"
#include "matrix3x3.hpp"

namespace SFG
{
	class quat;
	class matrix4x4;
	class ostream;
	class istream;

	class matrix4x3
	{
	public:
		f32 m[12]; // Column-major: m[col * 3 + row], 4 cols × 3 rows

		matrix4x3() = default;
		matrix4x3(f32 m00,
				  f32 m10,
				  f32 m20, // Col 0
				  f32 m01,
				  f32 m11,
				  f32 m21, // Col 1
				  f32 m02,
				  f32 m12,
				  f32 m22, // Col 2
				  f32 m03,
				  f32 m13,
				  f32 m23); // Col 3 (translation)

		static const matrix4x3 identity;

		inline vector3 get_translation() const
		{
			return vector3(m[9], m[10], m[11]);
		}

		vector3 get_scale() const;

		static matrix4x3 translation(const vector3& t);
		static matrix4x3 scale(const vector3& s);
		static matrix4x3 rotation(const quat& q);
		static matrix4x3 transform(const vector3& position, const quat& rotation, const vector3& scale);
		static matrix4x3 from_matrix4x4(const matrix4x4& mat);

		matrix4x3 inverse() const;
		void	  decompose(vector3& position, quat& rotation, vector3& scale) const;
		void	  serialize(ostream& stream) const;
		void	  deserialize(istream& stream);

		matrix4x4 to_matrix4x4() const;
		matrix3x3 to_linear3x3() const;

		inline f32 operator[](int index) const
		{
			return m[index];
		}
		inline f32& operator[](int index)
		{
			return m[index];
		}

		inline vector3 get_column(u8 idx) const
		{
			return vector3(m[idx * 3], m[idx * 3 + 1], m[idx * 3 + 2]);
		}

		inline vector4 get_column_v4(u8 idx) const
		{
			return vector4(m[idx * 3], m[idx * 3 + 1], m[idx * 3 + 2], 0.0f);
		}

		// Matrix × Matrix (composition)
		inline matrix4x3 operator*(const matrix4x3& other) const
		{
			matrix4x3 result;
			// 3x3 linear part
			for (int i = 0; i < 3; ++i) // row
			{
				for (int j = 0; j < 3; ++j) // col
				{
					result.m[j * 3 + i] = m[0 * 3 + i] * other.m[j * 3 + 0] + m[1 * 3 + i] * other.m[j * 3 + 1] + m[2 * 3 + i] * other.m[j * 3 + 2];
				}
			}
			// Translation
			for (int i = 0; i < 3; ++i)
			{
				result.m[3 * 3 + i] = m[0 * 3 + i] * other.m[9 + 0] + m[1 * 3 + i] * other.m[9 + 1] + m[2 * 3 + i] * other.m[9 + 2] + m[9 + i];
			}
			return result;
		}

		inline vector3 operator*(const vector3& v) const
		{
			return vector3(m[0] * v.x + m[3] * v.y + m[6] * v.z + m[9], m[1] * v.x + m[4] * v.y + m[7] * v.z + m[10], m[2] * v.x + m[5] * v.y + m[8] * v.z + m[11]);
		}

		inline matrix4x3 operator*(f32 scalar) const
		{
			matrix4x3 result;
			for (int i = 0; i < 12; ++i)
				result.m[i] = m[i] * scalar;
			return result;
		}

		inline matrix4x3& operator*=(f32 scalar)
		{
			for (int i = 0; i < 12; ++i)
				m[i] *= scalar;
			return *this;
		}
	};
}