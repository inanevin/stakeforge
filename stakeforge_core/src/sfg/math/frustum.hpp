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

#include "plane.hpp"

#undef near
#undef far

namespace sfg
{
	class mat4x4_t;
	class mat3x3_t;
	class vec3f_t;

	enum class frustum_result
	{
		inside,
		outside,
		intersects,
	};

	struct aabb_t;
	struct plane_t;

	struct frustum_t
	{
		static frustum_result test(const frustum_t& fr, const aabb_t& local_box);
		static frustum_result test(const frustum_t& fr, const vec3f_t& position, f32 sphere_radius);
		static frustum_result test(const frustum_t& fr, const aabb_t& local_box, const mat3x3_t& linear_model, const vec3f_t& position);
		static frustum_result classify_obb_vs_plane(const plane_t& p, const vec3f_t& c_local, const vec3f_t& e_local, const mat3x3_t& linear_model, const vec3f_t& position);
		static frustum_t	  extract(const mat4x4_t& view_proj);

		plane_t left   = {};
		plane_t right  = {};
		plane_t bottom = {};
		plane_t top	   = {};
		plane_t near   = {};
		plane_t far	   = {};
	};
}
