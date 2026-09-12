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

#include "aabb.hpp"
#include "plane.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	bool aabb_t::is_inside_plane(const vec3f_t& center, const plane_t& plane_t)
	{
		const f32 r = bounds_half_extent.x * math::abs(plane_t.normal.x) + bounds_half_extent.y * math::abs(plane_t.normal.y) + bounds_half_extent.z * math::abs(plane_t.normal.z);
		return -r <= plane_t.get_signed_distance(center);
	}

	vec3f_t aabb_t::get_positive(const vec3f_t& normal) const
	{
		vec3f_t positive = bounds_min;
		if (normal.x >= 0.0f)
			positive.x = bounds_max.x;
		if (normal.y >= 0.0f)
			positive.y = bounds_max.y;
		if (normal.z >= 0.0f)
			positive.z = bounds_max.z;

		return positive;
	}
	vec3f_t aabb_t::get_negative(const vec3f_t& normal) const
	{
		vec3f_t negative = bounds_max;
		if (normal.x >= 0.0f)
			negative.x = bounds_min.x;
		if (normal.y >= 0.0f)
			negative.y = bounds_min.y;
		if (normal.z >= 0.0f)
			negative.z = bounds_min.z;

		return negative;
	}

	void aabb_t::remove(const aabb_t& other)
	{
		bounds_min -= other.bounds_min;
		bounds_max -= other.bounds_max;
	}

	void aabb_t::add(const aabb_t& other)
	{
		bounds_min += other.bounds_min;
		bounds_max += other.bounds_max;
	}

	void aabb_t::serialize(ostream_t& stream) const
	{
		stream << bounds_min;
		stream << bounds_max;
	}
	void aabb_t::deserialize(istream_t& stream)
	{
		stream >> bounds_min;
		stream >> bounds_max;
		update_half_extents();
	}

	aabb_reflection_t::aabb_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "aabb_t",
			.fields =
				{
					{.name = "bounds_min", .display_name = "Min", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_min), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "bounds_max", .display_name = "Max", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_max), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "bounds_half_extent", .display_name = "Half Extent", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_half_extent), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<aabb_t>::value,
			.size	   = sizeof(aabb_t),
			.alignment = alignof(aabb_t),
		});
	}
}
