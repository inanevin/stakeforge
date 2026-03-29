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

#include "vec4f.hpp"
#include "math.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json.hpp"
#endif
namespace SFG
{
	const vec4f vec4f::zero(0.0f, 0.0f, 0.0f, 0.0f);
	const vec4f vec4f::one(1.0f, 1.0f, 1.0f, 1.0f);

	vec4f vec4f::clamp(const vec4f& vector_t, const vec4f& min_vec, const vec4f& max_vec)
	{
		return vec4f(math::clamp(vector_t.x, min_vec.x, max_vec.x), math::clamp(vector_t.y, min_vec.y, max_vec.y), math::clamp(vector_t.z, min_vec.z, max_vec.z), math::clamp(vector_t.w, min_vec.w, max_vec.w));
	}

	vec4f vec4f::abs(const vec4f& vector_t)
	{
		return vec4f(math::abs(vector_t.x), math::abs(vector_t.y), math::abs(vector_t.z), math::abs(vector_t.w));
	}

	vec4f vec4f::min(const vec4f& a, const vec4f& b)
	{
		return vec4f(math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z), math::min(a.w, b.w));
	}

	vec4f vec4f::max(const vec4f& a, const vec4f& b)
	{
		return vec4f(math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z), math::max(a.w, b.w));
	}

	f32 vec4f::dot(const vec4f& a, const vec4f& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	f32 vec4f::distance(const vec4f& a, const vec4f& b)
	{
		return (a - b).magnitude();
	}

	vec4f vec4f::project(const vec4f& on_normal) const
	{
		vec4f unit_normal = on_normal.normalized();
		if (unit_normal.is_zero())
		{
			return vec4f::zero;
		}
		return unit_normal * dot(*this, unit_normal);
	}

	vec4f vec4f::rotate(const vec4f& axis, f32 angle_degrees) const
	{
		vec4f unit_axis = axis.normalized();
		if (unit_axis.is_zero())
		{
			return *this;
		}

		f32 angle_rad = math::degrees_to_radians(angle_degrees);
		f32 cos_theta = math::cos(angle_rad);
		f32 sin_theta = math::sin(angle_rad);

		vec4f v_xyz = vec4f(x, y, z, 0.0f);
		vec4f k_xyz = vec4f(unit_axis.x, unit_axis.y, unit_axis.z, 0.0f);

		vec4f cross_kv = vec4f(k_xyz.y * v_xyz.z - k_xyz.z * v_xyz.y, k_xyz.z * v_xyz.x - k_xyz.x * v_xyz.z, k_xyz.x * v_xyz.y - k_xyz.y * v_xyz.x, 0.0f);

		f32 dot_kv = dot(k_xyz, v_xyz);

		vec4f rotated_xyz = (v_xyz * cos_theta) + (cross_kv * sin_theta) + (k_xyz * (dot_kv * (1.0f - cos_theta)));

		return vec4f(rotated_xyz.x, rotated_xyz.y, rotated_xyz.z, w);
	}
	f32 vec4f::magnitude() const
	{
		return math::sqrt(x * x + y * y + z * z + w * w);
	}

	f32 vec4f::magnitude_sqr() const
	{
		return x * x + y * y + z * z + w * w;
	}

	vec4f vec4f::operator/(f32 scalar) const
	{
		if (math::abs(scalar) < MATH_EPS)
			return vec4f::zero;
		return vec4f(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	vec4f& vec4f::operator/=(f32 scalar)
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
			x = y = z = w = MATH_INF_F;
		}
		return *this;
	}

	bool vec4f::equals(const vec4f& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon) && math::almost_equal(w, other.w, epsilon);
	}

	bool vec4f::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(z, 0.0f, epsilon) && math::almost_equal(w, 0.0f, epsilon);
	}

	void vec4f::serialize(ostream_t& stream) const
	{
		stream << x << y << z << w;
	}

	void vec4f::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z >> w;
	}

#ifdef SFG_JSON_SERIALIZE
	void to_json(nlohmann::json& j, const vec4f& v)
	{
		j = nlohmann::json::array({v.x, v.y, v.z, v.w});
	}

	void from_json(const nlohmann::json& j, vec4f& v)
	{
		if (!j.is_array() || j.size() < 4)
			throw std::runtime_error("vec4f json err");
		v.x = j.at(0).get<f32>();
		v.y = j.at(1).get<f32>();
		v.z = j.at(2).get<f32>();
		v.w = j.at(3).get<f32>();
	}

#endif
}
