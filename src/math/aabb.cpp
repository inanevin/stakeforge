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
#include "math/math.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	bool aabb::is_inside_plane(const vec3f& center, const plane& plane)
	{
		const f32 r = bounds_half_extent.x * math::abs(plane.normal.x) + bounds_half_extent.y * math::abs(plane.normal.y) + bounds_half_extent.z * math::abs(plane.normal.z);
		return -r <= plane.get_signed_distance(center);
	}
	vec3f aabb::get_positive(const vec3f& normal) const
	{
		vec3f positive = bounds_min;
		if (normal.x >= 0.0f)
			positive.x = bounds_max.x;
		if (normal.y >= 0.0f)
			positive.y = bounds_max.y;
		if (normal.z >= 0.0f)
			positive.z = bounds_max.z;

		return positive;
	}
	vec3f aabb::get_negative(const vec3f& normal) const
	{
		vec3f negative = bounds_max;
		if (normal.x >= 0.0f)
			negative.x = bounds_min.x;
		if (normal.y >= 0.0f)
			negative.y = bounds_min.y;
		if (normal.z >= 0.0f)
			negative.z = bounds_min.z;

		return negative;
	}

	void aabb::remove(const aabb& other)
	{
		bounds_min -= other.bounds_min;
		bounds_max -= other.bounds_max;
	}

	void aabb::add(const aabb& other)
	{
		bounds_min += other.bounds_min;
		bounds_max += other.bounds_max;
	}

	void aabb::serialize(ostream& stream) const
	{
		stream << bounds_min;
		stream << bounds_max;
	}
	void aabb::deserialize(istream& stream)
	{
		stream >> bounds_min;
		stream >> bounds_max;
		update_half_extents();
	}
}
