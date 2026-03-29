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
#include "vec4f.hpp"
#include "vec3f.hpp"
#include "mat3x3.hpp"

namespace SFG
{
	class quat;
	class mat4x4;
	class ostream_t;
	class istream_t;

	class mat4x3
	{
	public:
		f32 m[12]; // Column-major: m[col * 3 + row], 4 cols ? 3 rows

		mat4x3() = default;
		mat4x3(f32 m00,
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

		static const mat4x3 identity;

		inline vec3f get_translation() const
		{
			return vec3f(m[9], m[10], m[11]);
		}

		vec3f get_scale() const;

		static mat4x3 translation(const vec3f& t);
		static mat4x3 scale(const vec3f& s);
		static mat4x3 rotation(const quat& q);
		static mat4x3 transform(const vec3f& position, const quat& rotation, const vec3f& scale);
		static mat4x3 from_matrix4x4(const mat4x4& mat);

		mat4x3 inverse() const;
		void   decompose(vec3f& position, quat& rotation, vec3f& scale) const;
		void   serialize(ostream_t& stream) const;
		void   deserialize(istream_t& stream);

		mat4x4 to_matrix4x4() const;
		mat3x3 to_linear3x3() const;

		inline f32 operator[](int index) const
		{
			return m[index];
		}
		inline f32& operator[](int index)
		{
			return m[index];
		}

		inline vec3f get_column(u8 idx) const
		{
			return vec3f(m[idx * 3], m[idx * 3 + 1], m[idx * 3 + 2]);
		}

		inline vec4f get_column_v4(u8 idx) const
		{
			return vec4f(m[idx * 3], m[idx * 3 + 1], m[idx * 3 + 2], 0.0f);
		}

		// Matrix ? Matrix (composition)
		inline mat4x3 operator*(const mat4x3& other) const
		{
			mat4x3 result;
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

		inline vec3f operator*(const vec3f& v) const
		{
			return vec3f(m[0] * v.x + m[3] * v.y + m[6] * v.z + m[9], m[1] * v.x + m[4] * v.y + m[7] * v.z + m[10], m[2] * v.x + m[5] * v.y + m[8] * v.z + m[11]);
		}

		inline mat4x3 operator*(f32 scalar) const
		{
			mat4x3 result;
			for (int i = 0; i < 12; ++i)
				result.m[i] = m[i] * scalar;
			return result;
		}

		inline mat4x3& operator*=(f32 scalar)
		{
			for (int i = 0; i < 12; ++i)
				m[i] *= scalar;
			return *this;
		}
	};
}
