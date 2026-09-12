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

#include "render_view.hpp"
#include "world_render_view.hpp"
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	void render_view_t::calculate(const world_render_view_t& world_view, const vec2u16_t& resolution, f32 interpolation_alpha)
	{
		const vec3f_t p	  = vec3f_t::lerp(world_view.prev_pos, world_view.pos, interpolation_alpha);
		const quat_t  rot = quat_t::slerp(world_view.prev_rot, world_view.rot, interpolation_alpha);

		view		  = mat4x4_t::view(rot, p);
		inv_view	  = view.inverse();
		proj		  = mat4x4_t::perspective_reverse_z(world_view.fov_degrees, static_cast<f32>(resolution.x) / static_cast<f32>(resolution.y), world_view.near_plane, world_view.far_plane);
		view_proj	  = proj * view;
		frustum		  = frustum_t::extract(view_proj);
		inv_proj	  = proj.inverse();
		inv_view_proj = view_proj.inverse();
		pos			  = p;
		near_plane	  = world_view.near_plane;
		far_plane	  = world_view.far_plane;
		fov_rads	  = world_view.fov_degrees * DEG_2_RAD;
	}
}
