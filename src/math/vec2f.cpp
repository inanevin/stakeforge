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

#include "vec2f.hpp"
#include "vec2u16.hpp"
#include "math.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"
#include <limits>

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	vec2f vec2f::zero = vec2f(0.f, 0.f);
	vec2f vec2f::one  = vec2f(1.f, 1.f);

	vec2f::vec2f(const vec2u16& v)
	{
		x = static_cast<f32>(v.x);
		y = static_cast<f32>(v.y);
	}

	vec2f vec2f::clamp(const vec2f& vector, const vec2f& min_vec, const vec2f& max_vec)
	{
		return vec2f(math::max(min_vec.x, math::min(vector.x, max_vec.x)), math::max(min_vec.y, math::min(vector.y, max_vec.y)));
	}

	vec2f vec2f::clamp_magnitude(const vec2f& vector, f32 max_length)
	{
		f32 mag = vector.magnitude();
		if (mag > max_length)
		{
			return vector.normalized() * max_length;
		}
		return vector;
	}

	vec2f vec2f::abs(const vec2f& vector)
	{
		return vec2f(math::abs(vector.x), math::abs(vector.y));
	}

	vec2f vec2f::min(const vec2f& a, const vec2f& b)
	{
		return vec2f(math::min(a.x, b.x), math::min(a.y, b.y));
	}

	vec2f vec2f::max(const vec2f& a, const vec2f& b)
	{
		return vec2f(math::max(a.x, b.x), math::max(a.y, b.y));
	}

	f32 vec2f::dot(const vec2f& a, const vec2f& b)
	{
		return a.x * b.x + a.y * b.y;
	}

	f32 vec2f::distance(const vec2f& a, const vec2f& b)
	{
		return (a - b).magnitude();
	}

	f32 vec2f::angle(const vec2f& a, const vec2f& b)
	{
		f32 dot_product = dot(a, b);
		f32 magnitudes	= a.magnitude() * b.magnitude();

		if (magnitudes == 0.0f)
			return 0.0f;

		f32 cos_angle = dot_product / magnitudes;
		cos_angle	  = math::max(-1.0f, math::min(1.0f, cos_angle));

		return math::cos(cos_angle) * (180.0f / MATH_PI);
	}

	vec2f vec2f::normalized() const
	{
		f32 mag = magnitude();
		if (mag > MATH_EPS)
		{
			return vec2f(x / mag, y / mag);
		}
		return vec2f::zero;
	}

	bool vec2f::equals(const vec2f& other, f32 epsilon) const
	{
		return math::abs(x - other.x) < epsilon && math::abs(y - other.y) < epsilon;
	}

	bool vec2f::is_zero(f32 epsilon) const
	{
		return math::abs(x) < epsilon && math::abs(y) < epsilon;
	}

	f32 vec2f::magnitude() const
	{
		return math::sqrt(x * x + y * y);
	}

	f32 vec2f::magnitude_sqr() const
	{
		return x * x + y * y;
	}

	void vec2f::serialize(ostream& stream) const
	{
		stream << x << y;
	}

	void vec2f::deserialize(istream& stream)
	{
		stream >> x >> y;
	}

#ifdef SFG_JSON_SERIALIZE
	void to_json(nlohmann::json& j, const vec2f& v)
	{
		j = nlohmann::json::array({v.x, v.y});
	}

	void from_json(const nlohmann::json& j, vec2f& v)
	{
		if (!j.is_array() || j.size() < 2)
			throw std::runtime_error("vec2f json err");
		v.x = j.at(0).get<f32>();
		v.y = j.at(1).get<f32>();
	}

#endif
}
