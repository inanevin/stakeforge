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

#include "matrix3x3.hpp"
#include "matrix4x4.hpp"
#include "quat.hpp"
#include "math.hpp" // for sqrtf, etc.
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	matrix3x3::matrix3x3(f32 m00, f32 m10, f32 m20, f32 m01, f32 m11, f32 m21, f32 m02, f32 m12, f32 m22)
	{
		m[0] = m00;
		m[1] = m10;
		m[2] = m20; // Col 0
		m[3] = m01;
		m[4] = m11;
		m[5] = m21; // Col 1
		m[6] = m02;
		m[7] = m12;
		m[8] = m22; // Col 2
	}

	const matrix3x3 matrix3x3::identity(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	matrix3x3 matrix3x3::scale(const vector3& s)
	{
		return matrix3x3(s.x, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, s.z);
	}

	matrix3x3 matrix3x3::rotation(const quat& q)
	{
		const f32 x2 = q.x * q.x;
		const f32 y2 = q.y * q.y;
		const f32 z2 = q.z * q.z;
		const f32 xy = q.x * q.y;
		const f32 xz = q.x * q.z;
		const f32 yz = q.y * q.z;
		const f32 wx = q.w * q.x;
		const f32 wy = q.w * q.y;
		const f32 wz = q.w * q.z;

		return matrix3x3(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2));
	}

	matrix3x3 matrix3x3::from_axes(const vector3& x, const vector3& y, const vector3& z)
	{
		return matrix3x3(x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);
	}

	matrix4x4 matrix3x3::to_matrix4x4() const
	{
		return matrix4x4(m[0], m[1], m[2], 0.0f, m[3], m[4], m[5], 0.0f, m[6], m[7], m[8], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	matrix3x3 matrix3x3::transposed() const
	{
		return matrix3x3(m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]);
	}

	f32 matrix3x3::determinant() const
	{
		const f32 a = m[0], d = m[3], g = m[6];
		const f32 b = m[1], e = m[4], h = m[7];
		const f32 c = m[2], f = m[5], i = m[8];

		return a * (e * i - f * h) - d * (b * i - c * h) + g * (b * f - c * e);
	}

	matrix3x3 matrix3x3::inversed() const
	{
		const f32 a = m[0], d = m[3], g = m[6];
		const f32 b = m[1], e = m[4], h = m[7];
		const f32 c = m[2], f = m[5], i = m[8];

		const f32 A = (e * i - f * h);
		const f32 B = -(d * i - f * g);
		const f32 C = (d * h - e * g);

		const f32 D = -(b * i - c * h);
		const f32 E = (a * i - c * g);
		const f32 F = -(a * h - b * g);

		const f32 G = (b * f - c * e);
		const f32 H = -(a * f - c * d);
		const f32 I = (a * e - b * d);

		const f32 det = a * A + d * D + g * G;
		if (fabsf(det) <= 1e-8f)
			return identity;

		const f32 invDet = 1.0f / det;

		return matrix3x3(A * invDet, D * invDet, G * invDet, B * invDet, E * invDet, H * invDet, C * invDet, F * invDet, I * invDet);
	}

	matrix3x3 matrix3x3::abs(const matrix3x3& A)
	{
		matrix3x3 R;
		for (int i = 0; i < 9; ++i)
			R.m[i] = fabsf(A.m[i]);
		return R;
	}

	void matrix3x3::serialize(ostream& stream) const
	{
		for (int i = 0; i < 9; ++i)
			stream << m[i];
	}

	void matrix3x3::deserialize(istream& stream)
	{
		for (int i = 0; i < 9; ++i)
			stream >> m[i];
	}
}
