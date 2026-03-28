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

#include "mat4x4.hpp"
#include "math.hpp"
#include "quat.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	mat4x4::mat4x4(f32 m00,
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
				   f32 m33) // Col 3
	{
		m[0]  = m00;
		m[1]  = m10;
		m[2]  = m20;
		m[3]  = m30;
		m[4]  = m01;
		m[5]  = m11;
		m[6]  = m21;
		m[7]  = m31;
		m[8]  = m02;
		m[9]  = m12;
		m[10] = m22;
		m[11] = m32;
		m[12] = m03;
		m[13] = m13;
		m[14] = m23;
		m[15] = m33;
	}

	const mat4x4 mat4x4::identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	mat4x4 mat4x4::get_normal_matrix() const
	{
		mat4x4 inv_mat = inverse();
		if (inv_mat == mat4x4::identity)
			return identity;
		return inv_mat.transpose();
	}

	mat4x4 mat4x4::transpose() const
	{
		mat4x4 result;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result.m[i * 4 + j] = m[j * 4 + i]; // Swap col and row indices
			}
		}
		return result;
	}

	f32 mat4x4::determinant() const
	{
		f32 det;
		f32 a = m[0], b = m[4], c = m[8], d = m[12];
		f32 e = m[1], f = m[5], g = m[9], h = m[13];
		f32 i = m[2], j = m[6], k = m[10], l = m[14];
		f32 m_ = m[3], n = m[7], o = m[11], p = m[15];

		f32 kp_minus_lo = k * p - l * o;
		f32 jp_minus_ln = j * p - l * n;
		f32 jo_minus_kn = j * o - k * n;
		f32 ip_minus_lm = i * p - l * m_;
		f32 io_minus_km = i * o - k * m_;
		f32 in_minus_jm = i * n - j * m_;

		det = a * (f * kp_minus_lo - g * jp_minus_ln + h * jo_minus_kn) - b * (e * kp_minus_lo - g * ip_minus_lm + h * io_minus_km) + c * (e * jp_minus_ln - f * ip_minus_lm + h * in_minus_jm) - d * (e * jo_minus_kn - f * io_minus_km + g * in_minus_jm);

		return det;
	}

	mat4x4 mat4x4::inverse() const
	{
		f32 det = determinant();
		if (math::abs(det) < MATH_EPS)
			return identity;

		f32	   inv_det = 1.0f / det;
		mat4x4 inv;

		f32 a = m[0], b = m[4], c = m[8], d = m[12];
		f32 e = m[1], f = m[5], g = m[9], h = m[13];
		f32 i = m[2], j = m[6], k = m[10], l = m[14];
		f32 m_ = m[3], n = m[7], o = m[11], p = m[15];

		inv.m[0]  = (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n)) * inv_det;
		inv.m[4]  = (-b * (k * p - l * o) + c * (j * p - l * n) - d * (j * o - k * n)) * inv_det;
		inv.m[8]  = (b * (g * p - h * o) - c * (f * p - h * n) + d * (f * o - g * n)) * inv_det;
		inv.m[12] = (-b * (g * l - h * k) + c * (f * l - h * j) - d * (f * k - g * j)) * inv_det;
		inv.m[1]  = (-e * (k * p - l * o) + g * (i * p - l * m_) - h * (i * o - k * m_)) * inv_det;
		inv.m[5]  = (a * (k * p - l * o) - c * (i * p - l * m_) + d * (i * o - k * m_)) * inv_det;
		inv.m[9]  = (-a * (g * p - h * o) + c * (e * p - h * m_) - d * (e * o - g * m_)) * inv_det;
		inv.m[13] = (a * (g * l - h * k) - c * (e * l - h * i) + d * (e * k - g * i)) * inv_det;
		inv.m[2]  = (e * (j * p - l * n) - f * (i * p - l * m_) + h * (i * n - j * m_)) * inv_det;
		inv.m[6]  = (-a * (j * p - l * n) + b * (i * p - l * m_) - d * (i * n - j * m_)) * inv_det;
		inv.m[10] = (a * (f * p - h * n) - b * (e * p - h * m_) + d * (e * n - f * m_)) * inv_det;
		inv.m[14] = (-a * (f * l - h * j) + b * (e * l - h * i) - d * (e * j - f * i)) * inv_det;
		inv.m[3]  = (-e * (j * o - k * n) + f * (i * o - k * m_) - g * (i * n - j * m_)) * inv_det;
		inv.m[7]  = (a * (j * o - k * n) - b * (i * o - k * m_) + c * (i * n - j * m_)) * inv_det;
		inv.m[11] = (-a * (f * o - g * n) + b * (e * o - g * m_) - c * (e * n - f * m_)) * inv_det;
		inv.m[15] = (a * (f * k - g * j) - b * (e * k - g * i) + c * (e * j - f * i)) * inv_det;

		return inv;
	}

	vec3f mat4x4::get_scale() const
	{
		vec3f x_axis(m[0], m[1], m[2]);
		vec3f y_axis(m[4], m[5], m[6]);
		vec3f z_axis(m[8], m[9], m[10]);
		return vec3f(x_axis.magnitude(), y_axis.magnitude(), z_axis.magnitude());
	}

	vec3f mat4x4::get_translation() const
	{
		return vec3f(m[12], m[13], m[14]);
	}

	mat4x4 mat4x4::translation(const vec3f& t)
	{
		return mat4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, t.x, t.y, t.z, 1.0f);
	}

	mat4x4 mat4x4::scale(const vec3f& s)
	{
		return mat4x4(s.x, 0.0f, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, 0.0f, s.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	mat4x4 mat4x4::rotation(const quat& q)
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

		return mat4x4(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f, 2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 0.0f, 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	mat4x4 mat4x4::ortho_reverse_z(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
	{
		const f32 inv_width	 = 1.0f / (right - left);
		const f32 inv_height = 1.0f / (top - bottom);
		const f32 inv_depth	 = 1.0f / (far_plane - near_plane);

		// clang-format off
		return mat4x4(2.0f * inv_width, 0.0f, 0.0f, 0.0f, 
						0.0f, 2.0f * inv_height, 0.0f, 
						0.0f, 0.0f, 0.0f, -inv_depth, 
						0.0f, -(right + left) * inv_width, -(top + bottom) * inv_height, 1.0f + near_plane * inv_depth, 1.0f);
		// clang-format on
	}

	mat4x4 mat4x4::ortho(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
	{
		const f32 inv_width	 = 1.0f / (right - left);
		const f32 inv_height = 1.0f / (top - bottom);
		const f32 inv_depth	 = 1.0f / (near_plane - far_plane);

		// clang-format off
		return mat4x4(
						2.0f * inv_width,  0.0f,              0.0f, 0.0f,
						0.0f,              2.0f * inv_height, 0.0f, 0.0f,
						0.0f,              0.0f,              inv_depth, 0.0f,
						-(right + left) * inv_width,
						-(top + bottom)  * inv_height,
						 near_plane * inv_depth, 1.0f);
		// clang-format on
	}

	mat4x4 mat4x4::perspective_reverse_z(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane)
	{
		const f32 fov_rad	   = math::degrees_to_radians(fov_y_degrees);
		const f32 tan_half_fov = math::tan(0.5f * fov_rad);
		const f32 f			   = 1.0f / tan_half_fov;
		const f32 inv_nf	   = 1.0f / (far_plane - near_plane);

		// clang-format off
		return mat4x4(
			f / aspect_ratio, 0.0f, 0.0f, 0.0f, 
			0.0f, f, 0.0f, 0.0f, 
			0.0f, 0.0f, near_plane * inv_nf, -1.0f,
			0.0f, 0.0f, near_plane * far_plane * inv_nf, 0.0f);
		// clang-format on
	}

	mat4x4 mat4x4::perspective(f32 fov_y_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane)
	{
		const f32 fov_rad	   = math::degrees_to_radians(fov_y_degrees);
		const f32 tan_half_fov = math::tan(0.5f * fov_rad);
		const f32 f			   = 1.0f / tan_half_fov;
		const f32 inv_nf	   = 1.0f / (near_plane - far_plane);

		// clang-format off
		return mat4x4(
			f / aspect_ratio, 0.0f, 0.0f, 0.0f, 
			0.0f, f, 0.0f, 0.0f, 
			0.0f, 0.0f, far_plane * inv_nf, -1.0f,
			0.0f, 0.0f, near_plane * far_plane * inv_nf, 0.0f);
		// clang-format on
	}

	mat4x4 mat4x4::transform(const vec3f& position, const quat& rotation, const vec3f& scale_vec)
	{
		mat4x4 mat_s = mat4x4::scale(scale_vec);
		mat4x4 mat_r = mat4x4::rotation(rotation);
		mat4x4 mat_t = mat4x4::translation(position);

		return mat_t * mat_r * mat_s;
	}

	mat4x4 mat4x4::look_at(const vec3f& eye, const vec3f& target, const vec3f& up_vec)
	{
		vec3f  z_axis = (eye - target).normalized();
		vec3f  x_axis = vec3f::cross(up_vec, z_axis).normalized();
		vec3f  y_axis = vec3f::cross(z_axis, x_axis);
		mat4x4 result = mat4x4(x_axis.x, y_axis.x, z_axis.x, 0.0f, x_axis.y, y_axis.y, z_axis.y, 0.0f, x_axis.z, y_axis.z, z_axis.z, 0.0f, -vec3f::dot(x_axis, eye), -vec3f::dot(y_axis, eye), -vec3f::dot(z_axis, eye), 1.0f);
		return result;
	}

	mat4x4 mat4x4::view(const quat& rot, const vec3f& pos)
	{
		const mat4x4 rot_mat	 = mat4x4::rotation(rot.inverse());
		const mat4x4 translation = mat4x4::translation(-pos);
		return rot_mat * translation;
	}

	vec3f mat4x4::operator*(const vec3f& v) const
	{
		vec4f temp_v4(v.x, v.y, v.z, 1.0f);
		vec4f transformed_v4 = (*this) * temp_v4;
		if (math::abs(transformed_v4.w) < MATH_EPS)
			return vec3f(transformed_v4.x, transformed_v4.y, transformed_v4.z);
		return vec3f(transformed_v4.x / transformed_v4.w, transformed_v4.y / transformed_v4.w, transformed_v4.z / transformed_v4.w);
	}

	mat4x4 mat4x4::operator/(f32 scalar) const
	{
		mat4x4 result;
		if (math::abs(scalar) < MATH_EPS)
			return identity;
		f32 inv_scalar = 1.0f / scalar;
		for (int i = 0; i < 16; ++i)
		{
			result.m[i] = m[i] * inv_scalar;
		}
		return result;
	}

	mat4x4& mat4x4::operator/=(f32 scalar)
	{
		if (math::abs(scalar) < MATH_EPS)
		{
			for (int i = 0; i < 16; ++i)
				m[i] = std::numeric_limits<f32>::infinity();
		}
		else
		{
			f32 inv_scalar = 1.0f / scalar;
			for (int i = 0; i < 16; ++i)
			{
				m[i] *= inv_scalar;
			}
		}
		return *this;
	}

	bool mat4x4::equals(const mat4x4& other, f32 epsilon) const
	{
		for (int i = 0; i < 16; ++i)
		{
			if (!math::almost_equal(m[i], other.m[i], epsilon))
			{
				return false;
			}
		}
		return true;
	}

	void mat4x4::serialize(ostream& stream) const
	{
		for (int i = 0; i < 16; ++i)
			stream << m[i];
	}
	void mat4x4::deserialize(istream& stream)
	{
		for (int i = 0; i < 16; ++i)
			stream >> m[i];
	}

}
