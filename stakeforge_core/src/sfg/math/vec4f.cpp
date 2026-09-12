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

#include "vec4f.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "math.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>

namespace sfg
{
	const vec4f_t vec4f_t::zero = {0.0f, 0.0f, 0.0f, 0.0f};
	const vec4f_t vec4f_t::one	= {1.0f, 1.0f, 1.0f, 1.0f};

	vec4f_t vec4f_t::clamp(const vec4f_t& vector, const vec4f_t& min_vec, const vec4f_t& max_vec)
	{
		return {math::clamp(vector.x, min_vec.x, max_vec.x), math::clamp(vector.y, min_vec.y, max_vec.y), math::clamp(vector.z, min_vec.z, max_vec.z), math::clamp(vector.w, min_vec.w, max_vec.w)};
	}

	vec4f_t vec4f_t::abs(const vec4f_t& vector)
	{
		return {math::abs(vector.x), math::abs(vector.y), math::abs(vector.z), math::abs(vector.w)};
	}

	vec4f_t vec4f_t::min(const vec4f_t& a, const vec4f_t& b)
	{
		return {math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z), math::min(a.w, b.w)};
	}

	vec4f_t vec4f_t::max(const vec4f_t& a, const vec4f_t& b)
	{
		return {math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z), math::max(a.w, b.w)};
	}

	f32 vec4f_t::dot(const vec4f_t& a, const vec4f_t& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	f32 vec4f_t::distance(const vec4f_t& a, const vec4f_t& b)
	{
		return (a - b).magnitude();
	}

	vec4f_t vec4f_t::project(const vec4f_t& on_normal) const
	{
		vec4f_t unit_normal = on_normal.normalized();
		if (unit_normal.is_zero())
		{
			return vec4f_t::zero;
		}
		return unit_normal * dot(*this, unit_normal);
	}

	vec4f_t vec4f_t::rotate(const vec4f_t& axis, f32 angle_degrees) const
	{
		vec4f_t unit_axis = axis.normalized();
		if (unit_axis.is_zero())
		{
			return *this;
		}

		f32 angle_rad = math::degrees_to_radians(angle_degrees);
		f32 cos_theta = math::cos(angle_rad);
		f32 sin_theta = math::sin(angle_rad);

		vec4f_t v_xyz = {x, y, z, 0.0f};
		vec4f_t k_xyz = {unit_axis.x, unit_axis.y, unit_axis.z, 0.0f};

		vec4f_t cross_kv = {k_xyz.y * v_xyz.z - k_xyz.z * v_xyz.y, k_xyz.z * v_xyz.x - k_xyz.x * v_xyz.z, k_xyz.x * v_xyz.y - k_xyz.y * v_xyz.x, 0.0f};

		f32 dot_kv = dot(k_xyz, v_xyz);

		vec4f_t rotated_xyz = (v_xyz * cos_theta) + (cross_kv * sin_theta) + (k_xyz * (dot_kv * (1.0f - cos_theta)));

		return {rotated_xyz.x, rotated_xyz.y, rotated_xyz.z, w};
	}
	f32 vec4f_t::magnitude() const
	{
		return math::sqrt(x * x + y * y + z * z + w * w);
	}

	f32 vec4f_t::magnitude_sqr() const
	{
		return x * x + y * y + z * z + w * w;
	}

	vec4f_t vec4f_t::operator/(f32 scalar) const
	{
		if (math::abs(scalar) < MATH_EPS)
			return vec4f_t::zero;
		return {x / scalar, y / scalar, z / scalar, w / scalar};
	}

	vec4f_t& vec4f_t::operator/=(f32 scalar)
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

	bool vec4f_t::equals(const vec4f_t& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(z, other.z, epsilon) && math::almost_equal(w, other.w, epsilon);
	}

	bool vec4f_t::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(z, 0.0f, epsilon) && math::almost_equal(w, 0.0f, epsilon);
	}

	void vec4f_t::serialize(ostream_t& stream) const
	{
		stream << x << y << z << w;
	}

	void vec4f_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z >> w;
	}

}

namespace sfg
{
	vec4f_reflection_t::vec4f_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vec4f_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec4f_t, x), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "y", .offset = offsetof(vec4f_t, y), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "z", .offset = offsetof(vec4f_t, z), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "w", .offset = offsetof(vec4f_t, w), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<vec4f_t>::value,
			.size	   = sizeof(vec4f_t),
			.alignment = alignof(vec4f_t),
		});
	}
}
