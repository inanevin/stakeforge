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

#include "matrix4x3.hpp"
#include "matrix4x4.hpp"
#include "math.hpp"
#include "quat.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{

	mat4x3::mat4x3(f32 m00, f32 m10, f32 m20, f32 m01, f32 m11, f32 m21, f32 m02, f32 m12, f32 m22, f32 m03, f32 m13, f32 m23)
	{
		m[0]  = m00;
		m[1]  = m10;
		m[2]  = m20;
		m[3]  = m01;
		m[4]  = m11;
		m[5]  = m21;
		m[6]  = m02;
		m[7]  = m12;
		m[8]  = m22;
		m[9]  = m03;
		m[10] = m13;
		m[11] = m23;
	}

	const mat4x3 mat4x3::identity(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	vec3f mat4x3::get_scale() const
	{
		vec3f x_axis(m[0], m[1], m[2]);
		vec3f y_axis(m[3], m[4], m[5]);
		vec3f z_axis(m[6], m[7], m[8]);
		return vec3f(x_axis.magnitude(), y_axis.magnitude(), z_axis.magnitude());
	}

	mat4x3 mat4x3::translation(const vec3f& t)
	{
		return mat4x3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, t.x, t.y, t.z);
	}

	mat4x3 mat4x3::scale(const vec3f& s)
	{
		return mat4x3(s.x, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, s.z, 0.0f, 0.0f, 0.0f);
	}

	mat4x3 mat4x3::rotation(const quat& q)
	{
		f32 x2 = q.x * q.x;
		f32 y2 = q.y * q.y;
		f32 z2 = q.z * q.z;
		f32 xy = q.x * q.y;
		f32 xz = q.x * q.z;
		f32 yz = q.y * q.z;
		f32 wx = q.w * q.x;
		f32 wy = q.w * q.y;
		f32 wz = q.w * q.z;

		return mat4x3(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2), 0.0f, 0.0f, 0.0f);
	}

	mat4x3 mat4x3::transform(const vec3f& position, const quat& rotation, const vec3f& scale_vec)
	{
		mat4x3 mat_s = mat4x3::scale(scale_vec);
		mat4x3 mat_r = mat4x3::rotation(rotation);
		mat4x3 mat_t = mat4x3::translation(position);
		return mat_t * mat_r * mat_s;
	}

	mat4x3 mat4x3::inverse() const
	{
		mat4x3 result;

		vec3f s = get_scale();
		vec3f inv_s_sq(1.0f / (s.x * s.x), 1.0f / (s.y * s.y), 1.0f / (s.z * s.z));

		result.m[0] = m[0];
		result.m[1] = m[3];
		result.m[2] = m[6]; // Row 0 -> Col 0
		result.m[3] = m[1];
		result.m[4] = m[4];
		result.m[5] = m[7]; // Row 1 -> Col 1
		result.m[6] = m[2];
		result.m[7] = m[5];
		result.m[8] = m[8]; // Row 2 -> Col 2

		result.m[0] *= inv_s_sq.x;
		result.m[1] *= inv_s_sq.x;
		result.m[2] *= inv_s_sq.x;
		result.m[3] *= inv_s_sq.y;
		result.m[4] *= inv_s_sq.y;
		result.m[5] *= inv_s_sq.y;
		result.m[6] *= inv_s_sq.z;
		result.m[7] *= inv_s_sq.z;
		result.m[8] *= inv_s_sq.z;

		vec3f t(m[9], m[10], m[11]);

		vec3f inv_t;
		inv_t.x = result.m[0] * t.x + result.m[3] * t.y + result.m[6] * t.z;
		inv_t.y = result.m[1] * t.x + result.m[4] * t.y + result.m[7] * t.z;
		inv_t.z = result.m[2] * t.x + result.m[5] * t.y + result.m[8] * t.z;

		result.m[9]	 = -inv_t.x;
		result.m[10] = -inv_t.y;
		result.m[11] = -inv_t.z;

		return result;
	}

	void mat4x3::decompose(vec3f& out_translation, quat& out_rotation, vec3f& out_scale) const
	{
		out_translation = get_translation();

		// --- 2. Extract Scale ---
		vec3f x_axis(m[0], m[1], m[2]);
		vec3f y_axis(m[3], m[4], m[5]);
		vec3f z_axis(m[6], m[7], m[8]);

		out_scale.x = x_axis.magnitude();
		out_scale.y = y_axis.magnitude();
		out_scale.z = z_axis.magnitude();

		// Prevent divide by zero
		vec3f inv_scale(out_scale.x != 0.0f ? 1.0f / out_scale.x : 0.0f, out_scale.y != 0.0f ? 1.0f / out_scale.y : 0.0f, out_scale.z != 0.0f ? 1.0f / out_scale.z : 0.0f);

		// --- 3. Extract Rotation ---
		vec3f nx = x_axis * inv_scale.x;
		vec3f ny = y_axis * inv_scale.y;
		vec3f nz = z_axis * inv_scale.z;

		f32 trace = nx.x + ny.y + nz.z;

		if (trace > 0.0f)
		{
			f32 s		   = sqrtf(trace + 1.0f) * 2.0f;
			out_rotation.w = 0.25f * s;
			out_rotation.x = (ny.z - nz.y) / s;
			out_rotation.y = (nz.x - nx.z) / s;
			out_rotation.z = (nx.y - ny.x) / s;
		}
		else if (nx.x > ny.y && nx.x > nz.z)
		{
			f32 s		   = sqrtf(1.0f + nx.x - ny.y - nz.z) * 2.0f;
			out_rotation.w = (ny.z - nz.y) / s;
			out_rotation.x = 0.25f * s;
			out_rotation.y = (ny.x + nx.y) / s;
			out_rotation.z = (nz.x + nx.z) / s;
		}
		else if (ny.y > nz.z)
		{
			f32 s		   = sqrtf(1.0f + ny.y - nx.x - nz.z) * 2.0f;
			out_rotation.w = (nz.x - nx.z) / s;
			out_rotation.x = (ny.x + nx.y) / s;
			out_rotation.y = 0.25f * s;
			out_rotation.z = (nz.y + ny.z) / s;
		}
		else
		{
			f32 s		   = sqrtf(1.0f + nz.z - nx.x - ny.y) * 2.0f;
			out_rotation.w = (nx.y - ny.x) / s;
			out_rotation.x = (nz.x + nx.z) / s;
			out_rotation.y = (nz.y + ny.z) / s;
			out_rotation.z = 0.25f * s;
		}

		out_rotation.normalize();
	}

	mat4x4 mat4x3::to_matrix4x4() const
	{
		// Fill 4x4 with affine data
		return mat4x4(m[0],
						 m[1],
						 m[2],
						 0.0f, // Col 0
						 m[3],
						 m[4],
						 m[5],
						 0.0f, // Col 1
						 m[6],
						 m[7],
						 m[8],
						 0.0f, // Col 2
						 m[9],
						 m[10],
						 m[11],
						 1.0f // Col 3 (translation)
		);
	}

	mat4x3 mat4x3::from_matrix4x4(const mat4x4& mat)
	{
		return mat4x3(mat.m[0],
						 mat.m[1],
						 mat.m[2], // Col 0
						 mat.m[4],
						 mat.m[5],
						 mat.m[6], // Col 1
						 mat.m[8],
						 mat.m[9],
						 mat.m[10], // Col 2
						 mat.m[12],
						 mat.m[13],
						 mat.m[14] // Col 3
		);
	}

	mat3x3 mat4x3::to_linear3x3() const
	{
		return mat3x3(m[0],
						 m[1],
						 m[2], // Col 0
						 m[3],
						 m[4],
						 m[5], // Col 1
						 m[6],
						 m[7],
						 m[8]); // Col 2
	}

	void mat4x3::serialize(ostream& stream) const
	{
		for (int i = 0; i < 12; ++i)
			stream << m[i];
	}
	void mat4x3::deserialize(istream& stream)
	{
		for (int i = 0; i < 12; ++i)
			stream >> m[i];
	}
}
