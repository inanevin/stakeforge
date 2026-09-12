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

#include <sfg/math/frustum.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	struct vec2u16_t;
	struct world_render_view_t;

	struct render_view_t
	{
		frustum_t frustum		= {};
		mat4x4_t  view			= {};
		mat4x4_t  inv_view		= {};
		mat4x4_t  proj			= {};
		mat4x4_t  inv_proj		= {};
		mat4x4_t  view_proj		= {};
		mat4x4_t  inv_view_proj = {};
		vec3f_t	  pos			= vec3f_t::zero;
		f32		  near_plane	= 0.0f;
		f32		  far_plane		= 0.0f;
		f32		  fov_rads		= 0.0f;

		void calculate(const world_render_view_t& world_view, const vec2u16_t& resolution, f32 interpolation_alpha);
	};
}
