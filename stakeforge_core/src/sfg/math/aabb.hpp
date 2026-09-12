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

#pragma once
#include <sfg/common/type_id.hpp>

#include "vec3f.hpp"

namespace sfg
{
	struct plane_t;

	class ostream_t;
	class istream_t;

	struct aabb_t
	{
		aabb_t() = default;
		aabb_t(vec3f_t min, vec3f_t max)
		{
			bounds_min		   = min;
			bounds_max		   = max;
			bounds_half_extent = (max - min) / 2.0f;
		}
		~aabb_t() = default;

		vec3f_t bounds_half_extent = vec3f_t::zero;
		vec3f_t bounds_min		   = vec3f_t::zero;
		vec3f_t bounds_max		   = vec3f_t::zero;

		bool	is_inside_plane(const vec3f_t& center, const plane_t& plane_t);
		vec3f_t get_positive(const vec3f_t& normal) const;
		vec3f_t get_negative(const vec3f_t& normal) const;

		void remove(const aabb_t& other);
		void add(const aabb_t& other);
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline void update_half_extents()
		{
			bounds_half_extent = (bounds_max - bounds_min) / 2.0f;
		}

		inline bool is_empty() const
		{
			return bounds_half_extent.is_zero();
		}
	};

	SFG_DEFINE_TYPE_ID(aabb_t);

	struct aabb_reflection_t
	{
		aabb_reflection_t();
	};

	inline aabb_reflection_t g_reflect_aabb;
}
