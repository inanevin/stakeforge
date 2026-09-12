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
#include <sfg/math/quat.hpp>

namespace sfg
{
	struct world_render_view_t
	{
		vec3f_t pos			= vec3f_t::zero;
		quat_t	rot			= quat_t::identity;
		vec3f_t prev_pos	= vec3f_t::zero;
		quat_t	prev_rot	= quat_t::identity;
		f32		near_plane	= 0.0f;
		f32		far_plane	= 0.0f;
		f32		fov_degrees = 0.0f;
	};
}
