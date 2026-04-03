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

#include "vec3f.hpp"
#include "vec4f.hpp"

namespace sfg
{
	class quat_t;
	class ostream_t;
	class istream_t;
	class mat4x4_t;

	// Column-major 3x3: m[col * 3 + row]
	class mat3x3_t
	{
	public:
		f32 m[9];

		mat3x3_t() = default;

		mat3x3_t(f32 m00,
				 f32 m10,
				 f32 m20, // Col 0
				 f32 m01,
				 f32 m11,
				 f32 m21, // Col 1
				 f32 m02,
				 f32 m12,
				 f32 m22); // Col 2

		static const mat3x3_t identity;

		static mat3x3_t scale(const vec3f_t& s);
		static mat3x3_t rotation(const quat_t& q);
		static mat3x3_t from_axes(const vec3f_t& x, const vec3f_t& y, const vec3f_t& z);
		static mat3x3_t abs(const mat3x3_t& A);

		mat4x4_t to_matrix4x4() const;
		mat3x3_t transposed() const;
		mat3x3_t inversed() const;
		f32		 determinant() const;

		inline f32 operator[](int index) const
		{
			return m[index];
		}
		inline f32& operator[](int index)
		{
			return m[index];
		}

		inline vec3f_t get_column(int i) const
		{
			return vec3f_t(m[i * 3 + 0], m[i * 3 + 1], m[i * 3 + 2]);
		}

		inline vec4f_t get_column_v4(int i) const
		{
			return vec4f_t(m[i * 3 + 0], m[i * 3 + 1], m[i * 3 + 2], 0);
		}

		inline void set_column(int i, const vec3f_t& c)
		{
			m[i * 3 + 0] = c.x;
			m[i * 3 + 1] = c.y;
			m[i * 3 + 2] = c.z;
		}
		inline vec3f_t get_row(int i) const
		{
			return vec3f_t(m[0 * 3 + i], m[1 * 3 + i], m[2 * 3 + i]);
		}
		inline void set_row(int i, const vec3f_t& r)
		{
			m[0 * 3 + i] = r.x;
			m[1 * 3 + i] = r.y;
			m[2 * 3 + i] = r.z;
		}

		inline vec3f_t operator*(const vec3f_t& v) const
		{
			return vec3f_t(m[0] * v.x + m[3] * v.y + m[6] * v.z, m[1] * v.x + m[4] * v.y + m[7] * v.z, m[2] * v.x + m[5] * v.y + m[8] * v.z);
		}

		inline mat3x3_t operator*(const mat3x3_t& other) const
		{
			mat3x3_t r;
			for (int i = 0; i < 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					r.m[j * 3 + i] = m[0 * 3 + i] * other.m[j * 3 + 0] + m[1 * 3 + i] * other.m[j * 3 + 1] + m[2 * 3 + i] * other.m[j * 3 + 2];
				}
			}
			return r;
		}

		inline mat3x3_t operator*(f32 s) const
		{
			mat3x3_t r;
			for (int i = 0; i < 9; ++i)
				r.m[i] = m[i] * s;
			return r;
		}
		inline mat3x3_t& operator*=(f32 s)
		{
			for (int i = 0; i < 9; ++i)
				m[i] *= s;
			return *this;
		}

		// Utilities
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};
}
