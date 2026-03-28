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

#include "vector3.hpp"
#include "math.hpp"
#include "math/easing.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	const vec3f vec3f::zero(0.0f, 0.0f, 0.0f);
	const vec3f vec3f::one(1.0f, 1.0f, 1.0f);
	const vec3f vec3f::up(0.0f, 1.0f, 0.0f);
	const vec3f vec3f::forward(0.0f, 0.0f, -1.0f);
	const vec3f vec3f::right(1.0f, 0.0f, 0.0f);

	vec3f vec3f::clamp(const vec3f& vector, const vec3f& min_vec, const vec3f& max_vec)
	{
		return vec3f(math::clamp(vector.x, min_vec.x, max_vec.x), math::clamp(vector.y, min_vec.y, max_vec.y), math::clamp(vector.z, min_vec.z, max_vec.z));
	}

	vec3f vec3f::cross(const vec3f& a, const vec3f& b)
	{
		return vec3f(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}

	vec3f vec3f::abs(const vec3f& vector)
	{
		return vec3f(math::abs(vector.x), math::abs(vector.y), math::abs(vector.z));
	}

	vec3f vec3f::min(const vec3f& a, const vec3f& b)
	{
		return vec3f(math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z));
	}

	vec3f vec3f::max(const vec3f& a, const vec3f& b)
	{
		return vec3f(math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z));
	}

	vec3f vec3f::lerp(const vec3f& a, const vec3f& b, f32 t)
	{
		return vec3f(easing::lerp(a.x, b.x, t), easing::lerp(a.y, b.y, t), easing::lerp(a.z, b.z, t));
	}

	f32 vec3f::dot(const vec3f& a, const vec3f& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	f32 vec3f::distance(const vec3f& a, const vec3f& b)
	{
		return (a - b).magnitude();
	}

	f32 vec3f::distance_sqr(const vec3f& a, const vec3f& b)
	{
		return (a - b).magnitude_sqr();
	}

	vec3f vec3f::project(const vec3f& on_normal) const
	{
		return on_normal * dot(*this, on_normal);
	}

	vec3f vec3f::rotate(const vec3f& axis, f32 angle_degrees) const
	{
		vec3f unit_axis = axis.normalized();
		if (unit_axis.is_zero())
		{
			return *this;
		}

		f32 angle_rad = math::degrees_to_radians(angle_degrees);
		f32 cos_theta = math::cos(angle_rad);
		f32 sin_theta = math::sin(angle_rad);

		vec3f v_rot = (*this * cos_theta) + (vec3f::cross(unit_axis, *this) * sin_theta) + (unit_axis * (vec3f::dot(unit_axis, *this) * (1.0f - cos_theta)));
		return v_rot;
	}

	vec3f vec3f::reflect(const vec3f& in_normal) const
	{
		vec3f unit_normal = in_normal.normalized();
		if (unit_normal.is_zero())
		{
			return -(*this);
		}
		return *this - (unit_normal * (2.0f * vec3f::dot(*this, unit_normal)));
	}

	bool vec3f::equals(const vec3f& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon);
	}

	bool vec3f::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(z, 0.0f, epsilon);
	}

	f32 vec3f::magnitude() const
	{
		return math::sqrt(x * x + y * y + z * z);
	}

	f32 vec3f::magnitude_sqr() const
	{
		return x * x + y * y + z * z;
	}

	void vec3f::serialize(ostream& stream) const
	{
		stream << x << y << z;
	}

	void vec3f::deserialize(istream& stream)
	{
		stream >> x >> y >> z;
	}

#ifdef SFG_JSON_SERIALIZE
	void to_json(nlohmann::json& j, const vec3f& v)
	{
		j = nlohmann::json::array({v.x, v.y, v.z});
	}

	void from_json(const nlohmann::json& j, vec3f& v)
	{
		if (!j.is_array() || j.size() < 3)
			throw std::runtime_error("vec3f json err");

		v.x = j.at(0).get<f32>();
		v.y = j.at(1).get<f32>();
		v.z = j.at(2).get<f32>();
	}

#endif
}
