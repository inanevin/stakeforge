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

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	const vector3 vector3::zero(0.0f, 0.0f, 0.0f);
	const vector3 vector3::one(1.0f, 1.0f, 1.0f);
	const vector3 vector3::up(0.0f, 1.0f, 0.0f);
	const vector3 vector3::forward(0.0f, 0.0f, -1.0f);
	const vector3 vector3::right(1.0f, 0.0f, 0.0f);

	vector3 vector3::clamp(const vector3& vector, const vector3& min_vec, const vector3& max_vec)
	{
		return vector3(math::clamp(vector.x, min_vec.x, max_vec.x), math::clamp(vector.y, min_vec.y, max_vec.y), math::clamp(vector.z, min_vec.z, max_vec.z));
	}

	vector3 vector3::cross(const vector3& a, const vector3& b)
	{
		return vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}

	vector3 vector3::abs(const vector3& vector)
	{
		return vector3(math::abs(vector.x), math::abs(vector.y), math::abs(vector.z));
	}

	vector3 vector3::min(const vector3& a, const vector3& b)
	{
		return vector3(math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z));
	}

	vector3 vector3::max(const vector3& a, const vector3& b)
	{
		return vector3(math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z));
	}

	vector3 vector3::lerp(const vector3& a, const vector3& b, f32 t)
	{
		return vector3(easing::lerp(a.x, b.x, t), easing::lerp(a.y, b.y, t), easing::lerp(a.z, b.z, t));
	}

	f32 vector3::dot(const vector3& a, const vector3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	f32 vector3::distance(const vector3& a, const vector3& b)
	{
		return (a - b).magnitude();
	}

	f32 vector3::distance_sqr(const vector3& a, const vector3& b)
	{
		return (a - b).magnitude_sqr();
	}

	vector3 vector3::project(const vector3& on_normal) const
	{
		return on_normal * dot(*this, on_normal);
	}

	vector3 vector3::rotate(const vector3& axis, f32 angle_degrees) const
	{
		vector3 unit_axis = axis.normalized();
		if (unit_axis.is_zero())
		{
			return *this;
		}

		f32 angle_rad = math::degrees_to_radians(angle_degrees);
		f32 cos_theta = math::cos(angle_rad);
		f32 sin_theta = math::sin(angle_rad);

		vector3 v_rot = (*this * cos_theta) + (vector3::cross(unit_axis, *this) * sin_theta) + (unit_axis * (vector3::dot(unit_axis, *this) * (1.0f - cos_theta)));
		return v_rot;
	}

	vector3 vector3::reflect(const vector3& in_normal) const
	{
		vector3 unit_normal = in_normal.normalized();
		if (unit_normal.is_zero())
		{
			return -(*this);
		}
		return *this - (unit_normal * (2.0f * vector3::dot(*this, unit_normal)));
	}

	bool vector3::equals(const vector3& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon);
	}

	bool vector3::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(z, 0.0f, epsilon);
	}

	f32 vector3::magnitude() const
	{
		return math::sqrt(x * x + y * y + z * z);
	}

	f32 vector3::magnitude_sqr() const
	{
		return x * x + y * y + z * z;
	}

	void vector3::serialize(ostream& stream) const
	{
		stream << x << y << z;
	}

	void vector3::deserialize(istream& stream)
	{
		stream >> x >> y >> z;
	}

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const vector3& v)
	{
		j = nlohmann::json::array({v.x, v.y, v.z});
	}

	void from_json(const nlohmann::json& j, vector3& v)
	{
		if (!j.is_array() || j.size() < 3)
			throw std::runtime_error("vector3 json err");

		v.x = j.at(0).get<f32>();
		v.y = j.at(1).get<f32>();
		v.z = j.at(2).get<f32>();
	}

#endif
}
