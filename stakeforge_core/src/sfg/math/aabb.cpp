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

#include "aabb.hpp"
#include "plane.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

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
		if (registry.find_type(type_id_t<aabb_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "bounds_min", .display_name = "Min", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_min), .size = sizeof(vec3f_t)},
			{.name = "bounds_max", .display_name = "Max", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_max), .size = sizeof(vec3f_t)},
			{.name = "bounds_half_extent", .display_name = "Half Extent", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(aabb_t, bounds_half_extent), .size = sizeof(vec3f_t)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "aabb_t",
			.type_id   = type_id_t<aabb_t>::value,
			.size	   = sizeof(aabb_t),
			.alignment = alignof(aabb_t),
		});
	}
}
