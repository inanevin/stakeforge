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

#include "mat3x3.hpp"
#include "mat4x4.hpp"
#include "quat.hpp"
#include "math.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>

namespace sfg
{
	mat3x3_t::mat3x3_t(f32 m00, f32 m10, f32 m20, f32 m01, f32 m11, f32 m21, f32 m02, f32 m12, f32 m22)
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

	const mat3x3_t mat3x3_t::identity(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	mat3x3_t mat3x3_t::scale(const vec3f_t& s)
	{
		return mat3x3_t(s.x, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, s.z);
	}

	mat3x3_t mat3x3_t::rotation(const quat_t& q)
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

		return mat3x3_t(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2));
	}

	mat3x3_t mat3x3_t::from_axes(const vec3f_t& x, const vec3f_t& y, const vec3f_t& z)
	{
		return mat3x3_t(x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);
	}

	mat4x4_t mat3x3_t::to_matrix4x4() const
	{
		return mat4x4_t(m[0], m[1], m[2], 0.0f, m[3], m[4], m[5], 0.0f, m[6], m[7], m[8], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	mat3x3_t mat3x3_t::transposed() const
	{
		return mat3x3_t(m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]);
	}

	f32 mat3x3_t::determinant() const
	{
		const f32 a = m[0], d = m[3], g = m[6];
		const f32 b = m[1], e = m[4], h = m[7];
		const f32 c = m[2], f = m[5], i = m[8];

		return a * (e * i - f * h) - d * (b * i - c * h) + g * (b * f - c * e);
	}

	mat3x3_t mat3x3_t::inversed() const
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

		return mat3x3_t(A * invDet, D * invDet, G * invDet, B * invDet, E * invDet, H * invDet, C * invDet, F * invDet, I * invDet);
	}

	mat3x3_t mat3x3_t::abs(const mat3x3_t& A)
	{
		mat3x3_t R;
		for (int i = 0; i < 9; ++i)
			R.m[i] = fabsf(A.m[i]);
		return R;
	}

	void mat3x3_t::serialize(ostream_t& stream) const
	{
		for (int i = 0; i < 9; ++i)
			stream << m[i];
	}

	void mat3x3_t::deserialize(istream_t& stream)
	{
		for (int i = 0; i < 9; ++i)
			stream >> m[i];
	}
}
