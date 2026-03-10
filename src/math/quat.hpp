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

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostrem;
	class istream;
	// LH coordinates
	class quat
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 1.0f;

		quat() = default;
		quat(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w)
		{
		}

		static const quat identity;

		vector3 get_right() const;
		vector3 get_up() const;
		vector3 get_forward() const;
		quat	conjugate() const;
		quat	inverse() const;
		quat	normalized() const;
		f32		dot(const quat& other) const;
		f32		magnitude() const;
		f32		sqr_magnitude() const;
		void	normalize();
		bool	equals(const quat& other, f32 epsilon = MATH_EPS) const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		static quat	   from_euler(f32 pitch_degrees, f32 yaw_degrees, f32 roll_degrees);
		static vector3 to_euler(const quat& q);
		static quat	   angle_axis(f32 angle_degrees, const vector3& axis);
		static quat	   lerp(const quat& a, const quat& b, f32 t);
		static quat	   slerp(const quat& a, const quat& b, f32 t);
		static quat	   look_at(const vector3& source_point, const vector3& target_point, const vector3& up_vector);
		static quat	   from_rotation_matrix3x3(const f32 R_m[9]);

		inline bool is_identity(f32 epsilon = MATH_EPS) const
		{
			return equals(identity, epsilon);
		}

		inline quat operator+(const quat& other) const
		{
			return quat(x + other.x, y + other.y, z + other.z, w + other.w);
		}

		inline quat operator-(const quat& other) const
		{
			return quat(x - other.x, y - other.y, z - other.z, w - other.w);
		}

		inline quat operator*(const quat& other) const
		{
			return quat(w * other.x + x * other.w + y * other.z - z * other.y, w * other.y + y * other.w + z * other.x - x * other.z, w * other.z + z * other.w + x * other.y - y * other.x, w * other.w - x * other.x - y * other.y - z * other.z);
		}

		inline vector3 operator*(const vector3& v) const
		{
			quat p(v.x, v.y, v.z, 0.0f);
			quat q_inv	   = this->conjugate();
			quat rotated_p = (*this) * p * q_inv;
			return vector3(rotated_p.x, rotated_p.y, rotated_p.z);
		}

		inline quat operator*(f32 scalar) const
		{
			return quat(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		quat operator/(f32 scalar) const;

		inline quat operator-() const
		{
			return quat(-x, -y, -z, -w);
		}

		inline quat& operator*=(const quat& other)
		{
			*this = (*this) * other;
			return *this;
		}

		inline quat& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		quat& operator/=(f32 scalar);

		inline bool operator==(const quat& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const quat& other) const
		{
			return !equals(other);
		}
	};

	inline quat operator*(f32 scalar, const quat& q)
	{
		return q * scalar;
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const quat& q);
	void from_json(const nlohmann::json& j, quat& v);

#endif

}