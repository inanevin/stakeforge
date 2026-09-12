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

#include <sfg/math/vec3f.hpp>

namespace sfg
{
	class mat4x4_t;
	struct vec2f_t;

	struct world_ray_t
	{
		vec3f_t origin	  = vec3f_t::zero;
		vec3f_t direction = vec3f_t::forward;
	};

	class world_util_t final
	{
	public:
		static bool world_position_to_relative_position(const mat4x4_t& view_proj, const vec3f_t& world_position, vec2f_t& out_relative_position);
		static bool relative_position_to_world_position(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, f32 ndc_depth, vec3f_t& out_world_position);
		static bool relative_position_to_world_ray(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, world_ray_t& out_ray);
	};
}
