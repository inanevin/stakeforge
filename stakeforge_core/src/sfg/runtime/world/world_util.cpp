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

#include "world_util.hpp"

#include <sfg/math/mat4x4.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	bool world_util_t::world_position_to_relative_position(const mat4x4_t& view_proj, const vec3f_t& world_position, vec2f_t& out_relative_position)
	{
		const vec4f_t clip = view_proj * vec4f_t(world_position.x, world_position.y, world_position.z, 1.0f);

		if (clip.w <= MATH_EPS)
			return false;

		const f32 inv_w		  = 1.0f / clip.w;
		out_relative_position = {
			clip.x * inv_w * 0.5f + 0.5f,
			0.5f - clip.y * inv_w * 0.5f,
		};
		return true;
	}

	bool world_util_t::relative_position_to_world_position(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, f32 ndc_depth, vec3f_t& out_world_position)
	{
		const f32 ndc_x = relative_position.x * 2.0f - 1.0f;
		const f32 ndc_y = 1.0f - relative_position.y * 2.0f;
		vec4f_t	  world = inv_view_proj * vec4f_t(ndc_x, ndc_y, ndc_depth, 1.0f);

		if (math::abs(world.w) <= MATH_EPS)
			return false;

		world /= world.w;
		out_world_position = {world.x, world.y, world.z};
		return true;
	}

	bool world_util_t::relative_position_to_world_ray(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, world_ray_t& out_ray)
	{
		vec3f_t far_point = vec3f_t::zero;

		if (!relative_position_to_world_position(inv_view_proj, relative_position, 1.0f, out_ray.origin) || !relative_position_to_world_position(inv_view_proj, relative_position, 0.0f, far_point))
			return false;

		out_ray.direction = (far_point - out_ray.origin).normalized();
		return !out_ray.direction.is_zero();
	}
}
