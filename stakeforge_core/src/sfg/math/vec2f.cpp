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
#include <cstddef>
#include <sfg/reflection/reflection_registry_v2.hpp>
#include "vec2u16.hpp"
#include "math.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <limits>

namespace sfg
{
	vec2f_t vec2f_t::zero = {0.f, 0.f};
	vec2f_t vec2f_t::one  = {1.f, 1.f};

	vec2f_t vec2f_t::clamp(const vec2f_t& vector_t, const vec2f_t& min_vec, const vec2f_t& max_vec)
	{
		return {math::max(min_vec.x, math::min(vector_t.x, max_vec.x)), math::max(min_vec.y, math::min(vector_t.y, max_vec.y))};
	}

	vec2f_t vec2f_t::clamp_magnitude(const vec2f_t& vector_t, f32 max_length)
	{
		f32 mag = vector_t.magnitude();
		if (mag > max_length)
		{
			return vector_t.normalized() * max_length;
		}
		return vector_t;
	}

	vec2f_t vec2f_t::abs(const vec2f_t& vector_t)
	{
		return {math::abs(vector_t.x), math::abs(vector_t.y)};
	}

	vec2f_t vec2f_t::min(const vec2f_t& a, const vec2f_t& b)
	{
		return {math::min(a.x, b.x), math::min(a.y, b.y)};
	}

	vec2f_t vec2f_t::max(const vec2f_t& a, const vec2f_t& b)
	{
		return {math::max(a.x, b.x), math::max(a.y, b.y)};
	}

	f32 vec2f_t::dot(const vec2f_t& a, const vec2f_t& b)
	{
		return a.x * b.x + a.y * b.y;
	}

	f32 vec2f_t::distance(const vec2f_t& a, const vec2f_t& b)
	{
		return (a - b).magnitude();
	}

	f32 vec2f_t::angle(const vec2f_t& a, const vec2f_t& b)
	{
		f32 dot_product = dot(a, b);
		f32 magnitudes	= a.magnitude() * b.magnitude();

		if (magnitudes == 0.0f)
			return 0.0f;

		f32 cos_angle = dot_product / magnitudes;
		cos_angle	  = math::max(-1.0f, math::min(1.0f, cos_angle));

		return math::cos(cos_angle) * (180.0f / MATH_PI);
	}

	vec2f_t vec2f_t::normalized() const
	{
		f32 mag = magnitude();
		if (mag > MATH_EPS)
		{
			return {x / mag, y / mag};
		}
		return vec2f_t::zero;
	}

	bool vec2f_t::equals(const vec2f_t& other, f32 epsilon) const
	{
		return math::abs(x - other.x) < epsilon && math::abs(y - other.y) < epsilon;
	}

	bool vec2f_t::is_zero(f32 epsilon) const
	{
		return math::abs(x) < epsilon && math::abs(y) < epsilon;
	}

	f32 vec2f_t::magnitude() const
	{
		return math::sqrt(x * x + y * y);
	}

	f32 vec2f_t::magnitude_sqr() const
	{
		return x * x + y * y;
	}

	void vec2f_t::serialize(ostream_t& stream) const
	{
		stream << x << y;
	}

	void vec2f_t::deserialize(istream_t& stream)
	{
		stream >> x >> y;
	}

}

namespace sfg
{
	vec2f_reflection_t::vec2f_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "vec2f_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec2f_t, x), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
					{.name = "y", .offset = offsetof(vec2f_t, y), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
				},
			.type_id   = type_id_t<vec2f_t>::value,
			.size	   = sizeof(vec2f_t),
			.alignment = alignof(vec2f_t),
		});
	}
}
