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
#include "common/size_definitions.hpp"

#undef min
#undef max

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class istream_t;
	class ostream_t;

	struct vec2u16_t;

	class vec2f_t
	{
	public:
		vec2f_t(){};
		vec2f_t(f32 _x, f32 _y) : x(_x), y(_y){};
		vec2f_t(const vec2u16_t& v);

		f32 x = 0.0f;
		f32 y = 0.0f;

		static vec2f_t zero;
		static vec2f_t one;

		static vec2f_t clamp(const vec2f_t& vector_t, const vec2f_t& min, const vec2f_t& max);
		static vec2f_t clamp_magnitude(const vec2f_t& vector_t, f32 max_length);
		static vec2f_t abs(const vec2f_t& vector_t);
		static vec2f_t min(const vec2f_t& a, const vec2f_t& b);
		static vec2f_t max(const vec2f_t& a, const vec2f_t& b);
		static f32	   dot(const vec2f_t& a, const vec2f_t& b);
		static f32	   distance(const vec2f_t& a, const vec2f_t& b);
		static f32	   angle(const vec2f_t& a, const vec2f_t& b);

		vec2f_t normalized() const;
		bool	equals(const vec2f_t& other, f32 epsilon = MATH_EPS) const;
		bool	is_zero(f32 epsilon = MATH_EPS) const;
		f32		magnitude() const;
		f32		magnitude_sqr() const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline vec2f_t operator+(const vec2f_t& other) const
		{
			return vec2f_t(x + other.x, y + other.y);
		}
		inline vec2f_t operator-(const vec2f_t& other) const
		{
			return vec2f_t(x - other.x, y - other.y);
		}
		inline vec2f_t operator*(f32 scalar) const
		{
			return vec2f_t(x * scalar, y * scalar);
		}

		inline vec2f_t operator/(f32 scalar) const
		{
			if (scalar == 0.0f)
			{
				return vec2f_t(std::numeric_limits<f32>::infinity(), std::numeric_limits<f32>::infinity());
			}
			return vec2f_t(x / scalar, y / scalar);
		}

		inline vec2f_t& operator+=(const vec2f_t& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		inline vec2f_t& operator-=(const vec2f_t& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		inline vec2f_t& operator*=(f32 scalar)
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		inline vec2f_t& operator/=(f32 scalar)
		{
			if (scalar == 0.0f)
			{
				x = MATH_INF_F;
				y = MATH_INF_F;
			}
			else
			{
				x /= scalar;
				y /= scalar;
			}
			return *this;
		}

		inline bool operator==(const vec2f_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const vec2f_t& other) const
		{
			return !(*this == other);
		}
	};

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const vec2f_t& v);
	void from_json(const nlohmann::json& j, vec2f_t& v);

#endif
}
