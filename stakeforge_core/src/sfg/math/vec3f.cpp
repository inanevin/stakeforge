/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "vec3f.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "math.hpp"
#include <sfg/math/easing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>

namespace sfg
{
	const vec3f_t vec3f_t::zero	   = {0.0f, 0.0f, 0.0f};
	const vec3f_t vec3f_t::one	   = {1.0f, 1.0f, 1.0f};
	const vec3f_t vec3f_t::up	   = {0.0f, 1.0f, 0.0f};
	const vec3f_t vec3f_t::forward = {0.0f, 0.0f, -1.0f};
	const vec3f_t vec3f_t::right   = {1.0f, 0.0f, 0.0f};

	vec3f_t vec3f_t::clamp(const vec3f_t& vector_t, const vec3f_t& min_vec, const vec3f_t& max_vec)
	{
		return {math::clamp(vector_t.x, min_vec.x, max_vec.x), math::clamp(vector_t.y, min_vec.y, max_vec.y), math::clamp(vector_t.z, min_vec.z, max_vec.z)};
	}

	vec3f_t vec3f_t::cross(const vec3f_t& a, const vec3f_t& b)
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	vec3f_t vec3f_t::abs(const vec3f_t& vector_t)
	{
		return {math::abs(vector_t.x), math::abs(vector_t.y), math::abs(vector_t.z)};
	}

	vec3f_t vec3f_t::min(const vec3f_t& a, const vec3f_t& b)
	{
		return {math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z)};
	}

	vec3f_t vec3f_t::max(const vec3f_t& a, const vec3f_t& b)
	{
		return {math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z)};
	}

	vec3f_t vec3f_t::lerp(const vec3f_t& a, const vec3f_t& b, f32 t)
	{
		return {easing_t::lerp(a.x, b.x, t), easing_t::lerp(a.y, b.y, t), easing_t::lerp(a.z, b.z, t)};
	}

	f32 vec3f_t::dot(const vec3f_t& a, const vec3f_t& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	f32 vec3f_t::distance(const vec3f_t& a, const vec3f_t& b)
	{
		return (a - b).magnitude();
	}

	f32 vec3f_t::distance_sqr(const vec3f_t& a, const vec3f_t& b)
	{
		return (a - b).magnitude_sqr();
	}

	vec3f_t vec3f_t::project(const vec3f_t& on_normal) const
	{
		return on_normal * dot(*this, on_normal);
	}

	vec3f_t vec3f_t::rotate(const vec3f_t& axis, f32 angle_degrees) const
	{
		vec3f_t unit_axis = axis.normalized();
		if (unit_axis.is_zero())
		{
			return *this;
		}

		f32 angle_rad = math::degrees_to_radians(angle_degrees);
		f32 cos_theta = math::cos(angle_rad);
		f32 sin_theta = math::sin(angle_rad);

		vec3f_t v_rot = (*this * cos_theta) + (vec3f_t::cross(unit_axis, *this) * sin_theta) + (unit_axis * (vec3f_t::dot(unit_axis, *this) * (1.0f - cos_theta)));
		return v_rot;
	}

	vec3f_t vec3f_t::reflect(const vec3f_t& in_normal) const
	{
		vec3f_t unit_normal = in_normal.normalized();
		if (unit_normal.is_zero())
		{
			return -(*this);
		}
		return *this - (unit_normal * (2.0f * vec3f_t::dot(*this, unit_normal)));
	}

	bool vec3f_t::equals(const vec3f_t& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon);
	}

	bool vec3f_t::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(z, 0.0f, epsilon);
	}

	f32 vec3f_t::magnitude() const
	{
		return math::sqrt(x * x + y * y + z * z);
	}

	f32 vec3f_t::magnitude_sqr() const
	{
		return x * x + y * y + z * z;
	}

	void vec3f_t::serialize(ostream_t& stream) const
	{
		stream << x << y << z;
	}

	void vec3f_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z;
	}

}

namespace sfg
{
	vec3f_reflection_t::vec3f_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vec3f_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec3f_t, x), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "y", .offset = offsetof(vec3f_t, y), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "z", .offset = offsetof(vec3f_t, z), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<vec3f_t>::value,
			.size	   = sizeof(vec3f_t),
			.alignment = alignof(vec3f_t),
		});
	}
}
