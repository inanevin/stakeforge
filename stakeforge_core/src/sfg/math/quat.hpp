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

#include "vendor/nhlohmann/json_fwd.hpp"

namespace sfg
{
	class ostrem;
	class istream_t;
	// LH coordinates
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

	void to_json(nlohmann::json& j, const quat_t& q);
	void from_json(const nlohmann::json& j, quat_t& v);

}
