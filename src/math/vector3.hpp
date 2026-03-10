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
#include "math_common.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

#undef min
#undef max

namespace SFG
{
	class istream;
	class ostream;

	class vector3
	{
	public:
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;

		vector3() = default;
		vector3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z)
		{
		}

		static const vector3 zero;
		static const vector3 one;
		static const vector3 up;
		static const vector3 forward;
		static const vector3 right;

		static vector3 clamp(const vector3& vector, const vector3& min, const vector3& max);
		static vector3 cross(const vector3& a, const vector3& b);
		static vector3 abs(const vector3& vector);
		static vector3 min(const vector3& a, const vector3& b);
		static vector3 max(const vector3& a, const vector3& b);
		static vector3 lerp(const vector3& a, const vector3& b, f32 t);
		static f32	   dot(const vector3& a, const vector3& b);
		static f32	   distance(const vector3& a, const vector3& b);
		static f32	   distance_sqr(const vector3& a, const vector3& b);
		vector3		   project(const vector3& on_normal) const;
		vector3		   rotate(const vector3& axis, f32 angle_degrees) const;
		vector3		   reflect(const vector3& in_normal) const;
		bool		   equals(const vector3& other, f32 epsilon = MATH_EPS) const;
		bool		   is_zero(f32 epsilon = MATH_EPS) const;
		f32			   magnitude() const;
		f32			   magnitude_sqr() const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline vector3 normalized() const
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				return vector3(x / mag, y / mag, z / mag);
			}
			return vector3::zero;
		}

		inline void normalize()
		{
			f32 mag = magnitude();
			if (mag > MATH_EPS)
			{
				x /= mag;
				y /= mag;
				z /= mag;
			}
			else
				x = y = z = 0.0f;
		}

		inline vector3 operator+(const vector3& other) const
		{
			return vector3(x + other.x, y + other.y, z + other.z);
		}
		inline vector3 operator-(const vector3& other) const
		{
			return vector3(x - other.x, y - other.y, z - other.z);
		}
		inline vector3 operator*(f32 scalar) const
		{
			return vector3(x * scalar, y * scalar, z * scalar);
		}
		inline vector3 operator/(f32 scalar) const
		{
			if (scalar == 0.0f)
				return vector3::zero;
			return vector3(x / scalar, y / scalar, z / scalar);
		}

		inline vector3 operator*(const vector3& other) const
		{
			return vector3(x * other.x, y * other.y, z * other.z);
		}

		inline vector3 operator-() const
		{
			return vector3(-x, -y, -z);
		}

		inline vector3& operator+=(const vector3& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}
		inline vector3& operator-=(const vector3& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}
		inline vector3& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		inline vector3& operator/=(f32 scalar)
		{
			if (scalar != 0.0f)
			{
				x /= scalar;
				y /= scalar;
				z /= scalar;
			}
			else
			{
				x = MATH_INF_F;
				y = MATH_INF_F;
				z = MATH_INF_F;
			}
			return *this;
		}

		inline bool operator==(const vector3& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vector3& other) const
		{
			return !equals(other);
		}
	};

	inline vector3 operator*(f32 scalar, const vector3& vector)
	{
		return vector * scalar;
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const vector3& v);
	void from_json(const nlohmann::json& j, vector3& v);

#endif
}
