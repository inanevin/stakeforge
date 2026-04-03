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

#include "quat.hpp"
#include "math.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json.hpp"
#endif

namespace sfg
{
	const quat_t quat_t::identity(0.0f, 0.0f, 0.0f, 1.0f);

	vec3f_t quat_t::get_right() const
	{
		return (*this) * vec3f_t::right;
	}

	vec3f_t quat_t::get_up() const
	{
		return (*this) * vec3f_t::up;
	}

	vec3f_t quat_t::get_forward() const
	{
		return (*this) * vec3f_t::forward;
	}

	quat_t quat_t::conjugate() const
	{
		return quat_t(-x, -y, -z, w);
	}

	quat_t quat_t::inverse() const
	{
		f32 mag_sqr = sqr_magnitude();
		if (math::abs(mag_sqr) < MATH_EPS)
			return identity;
		return conjugate() / mag_sqr;
	}

	quat_t quat_t::normalized() const
	{
		f32 mag = magnitude();
		if (math::abs(mag) < MATH_EPS)
			return identity;
		return (*this) / mag;
	}

	void quat_t::normalize()
	{
		*this = normalized();
	}

	f32 quat_t::dot(const quat_t& other) const
	{
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	f32 quat_t::magnitude() const
	{
		return std::sqrt(sqr_magnitude());
	}

	f32 quat_t::sqr_magnitude() const
	{
		return x * x + y * y + z * z + w * w;
	}

	quat_t quat_t::from_euler(f32 pitch_degrees, f32 yaw_degrees, f32 roll_degrees)
	{
		f32 pitch_rad = math::degrees_to_radians(pitch_degrees); // X-axis
		f32 yaw_rad	  = math::degrees_to_radians(yaw_degrees);	 // Y-axis
		f32 roll_rad  = math::degrees_to_radians(roll_degrees);	 // Z-axis

		f32 cx = math::cos(pitch_rad * 0.5f);
		f32 sx = math::sin(pitch_rad * 0.5f);
		f32 cy = math::cos(yaw_rad * 0.5f);
		f32 sy = math::sin(yaw_rad * 0.5f);
		f32 cz = math::cos(roll_rad * 0.5f);
		f32 sz = math::sin(roll_rad * 0.5f);

		quat_t q;
		// Z-Y-X Order: q = Q_z * Q_y * Q_x
		q.w = cx * cy * cz + sx * sy * sz;
		q.x = sx * cy * cz - cx * sy * sz;
		q.y = cx * sy * cz + sx * cy * sz;
		q.z = cx * cy * sz - sx * sy * cz;

		return q;
	}

	vec3f_t quat_t::to_euler(const quat_t& q)
	{
		vec3f_t e;

		// X (pitch)
		f32 sinp = 2.0f * (q.w * q.x + q.y * q.z);
		f32 cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
		e.x		 = math::radians_to_degrees(std::atan2(sinp, cosp));

		// Y (yaw)
		f32 siny = 2.0f * (q.w * q.y - q.z * q.x);
		siny	 = math::clamp(siny, -1.0f, 1.0f);
		e.y		 = math::radians_to_degrees(std::asin(siny));

		// Z (roll)
		f32 sinr = 2.0f * (q.w * q.z + q.x * q.y);
		f32 cosr = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
		e.z		 = math::radians_to_degrees(std::atan2(sinr, cosr));

		return e;
	}

	quat_t quat_t::angle_axis(f32 angle_degrees, const vec3f_t& axis)
	{
		f32		angle_rad_half	= math::degrees_to_radians(angle_degrees * 0.5f);
		f32		s				= math::sin(angle_rad_half);
		vec3f_t normalized_axis = axis.normalized();
		return quat_t(normalized_axis.x * s, normalized_axis.y * s, normalized_axis.z * s, math::cos(angle_rad_half));
	}

	quat_t quat_t::lerp(const quat_t& a, const quat_t& b, f32 t)
	{
		f32	   dot_product = a.dot(b);
		quat_t result	   = b;

		if (dot_product < 0.0f)
		{
			result.x = -b.x;
			result.y = -b.y;
			result.z = -b.z;
			result.w = -b.w;
		}

		return (a * (1.0f - t) + (result * t)).normalized();
	}

	quat_t quat_t::slerp(const quat_t& a, const quat_t& b, f32 t)
	{
		f32	   dot_product = a.dot(b);
		quat_t b_adjusted  = b;

		if (dot_product < 0.0f)
		{
			dot_product	 = -dot_product;
			b_adjusted.x = -b.x;
			b_adjusted.y = -b.y;
			b_adjusted.z = -b.z;
			b_adjusted.w = -b.w;
		}

		if (dot_product > 0.9995f)
		{
			return lerp(a, b_adjusted, t);
		}

		f32 theta	  = std::acos(dot_product);
		f32 sin_theta = math::sin(theta);

		f32 s0 = math::sin((1.0f - t) * theta) / sin_theta;
		f32 s1 = math::sin(t * theta) / sin_theta;

		return (a * s0) + (b_adjusted * s1);
	}

	quat_t quat_t::look_at(const vec3f_t& source_point, const vec3f_t& target_point, const vec3f_t& up_vector)
	{
		vec3f_t forward_vec	 = (target_point - source_point).normalized();
		vec3f_t right_vec	 = vec3f_t::cross(up_vector, forward_vec).normalized();
		vec3f_t final_up_vec = vec3f_t::cross(forward_vec, right_vec);

		f32 m00 = right_vec.x;
		f32 m01 = final_up_vec.x;
		f32 m02 = forward_vec.x;
		f32 m10 = right_vec.y;
		f32 m11 = final_up_vec.y;
		f32 m12 = forward_vec.y;
		f32 m20 = right_vec.z;
		f32 m21 = final_up_vec.z;
		f32 m22 = forward_vec.z;

		quat_t q;
		f32	   trace = m00 + m11 + m22;

		if (trace > 0.0f)
		{
			f32 s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w	  = 0.25f * s;
			q.x	  = (m21 - m12) / s;
			q.y	  = (m02 - m20) / s;
			q.z	  = (m10 - m01) / s;
		}
		else if ((m00 > m11) && (m00 > m22))
		{
			f32 s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q.w	  = (m21 - m12) / s;
			q.x	  = 0.25f * s;
			q.y	  = (m01 + m10) / s;
			q.z	  = (m02 + m20) / s;
		}
		else if (m11 > m22)
		{
			f32 s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q.w	  = (m02 - m20) / s;
			q.x	  = (m01 + m10) / s;
			q.y	  = 0.25f * s;
			q.z	  = (m12 + m21) / s;
		}
		else
		{
			f32 s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q.w	  = (m10 - m01) / s;
			q.x	  = (m02 + m20) / s;
			q.y	  = (m12 + m21) / s;
			q.z	  = 0.25f * s;
		}

		return q.normalized();
	}

	quat_t quat_t::from_rotation_matrix3x3(const f32 R_m[9])
	{
		f32	   trace = R_m[0] + R_m[4] + R_m[8]; // R[0,0] + R[1,1] + R[2,2]
		quat_t q;

		if (trace > 0.0f)
		{
			f32 s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4w
			q.w	  = 0.25f * s;
			q.x	  = (R_m[7] - R_m[5]) / s;
			q.y	  = (R_m[2] - R_m[6]) / s;
			q.z	  = (R_m[3] - R_m[1]) / s;
		}
		else if ((R_m[0] > R_m[4]) && (R_m[0] > R_m[8]))
		{
			f32 s = std::sqrt(1.0f + R_m[0] - R_m[4] - R_m[8]) * 2.0f;
			q.w	  = (R_m[7] - R_m[5]) / s;
			q.x	  = 0.25f * s;
			q.y	  = (R_m[3] + R_m[1]) / s;
			q.z	  = (R_m[2] + R_m[6]) / s;
		}
		else if (R_m[4] > R_m[8])
		{
			f32 s = std::sqrt(1.0f + R_m[4] - R_m[0] - R_m[8]) * 2.0f;
			q.w	  = (R_m[2] - R_m[6]) / s;
			q.x	  = (R_m[3] + R_m[1]) / s;
			q.y	  = 0.25f * s;
			q.z	  = (R_m[7] + R_m[5]) / s;
		}
		else
		{
			f32 s = std::sqrt(1.0f + R_m[8] - R_m[0] - R_m[4]) * 2.0f;
			q.w	  = (R_m[3] - R_m[1]) / s;
			q.x	  = (R_m[2] + R_m[6]) / s;
			q.y	  = (R_m[7] + R_m[5]) / s;
			q.z	  = 0.25f * s;
		}

		return q.normalized();
	}

	quat_t quat_t::operator/(f32 scalar) const
	{
		if (math::abs(scalar) < MATH_EPS)
			return identity;
		return quat_t(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	quat_t& quat_t::operator/=(f32 scalar)
	{
		if (math::abs(scalar) > MATH_EPS)
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
		}
		else
		{
			x = y = z = w = MATH_NAN;
		}
		return *this;
	}

	bool quat_t::equals(const quat_t& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon) && math::almost_equal(w, other.w, epsilon);
	}

	void quat_t::serialize(ostream_t& stream) const
	{
		stream << x << y << z << w;
	}
	void quat_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z >> w;
	}

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const quat_t& q)
	{
		j = nlohmann::json::array_t({q.x, q.y, q.z, q.w});
	}

	void from_json(const nlohmann::json& j, quat_t& q)
	{
		if (!j.is_array() || j.size() < 4)
			throw std::runtime_error("quat json err");
		q.x = j.at(0).get<f32>();
		q.y = j.at(1).get<f32>();
		q.z = j.at(2).get<f32>();
		q.w = j.at(3).get<f32>();
	}
#endif

}
