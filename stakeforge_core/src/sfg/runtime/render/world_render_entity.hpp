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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct world_render_entity_t
	{
		entity_id_t entity_id	   = NULL_ENTITY_ID;
		mat4x3_t	prev_transform = mat4x3_t::identity;
		mat4x3_t	transform	   = mat4x3_t::identity;
		quat_t		prev_rot	   = quat_t::identity;
		quat_t		rot			   = quat_t::identity;
		vec3f_t		prev_pos	   = vec3f_t::zero;
		vec3f_t		prev_scale	   = vec3f_t::one;
		vec3f_t		pos			   = vec3f_t::zero;
		vec3f_t		scale		   = vec3f_t::one;
		u32			render_id	   = 0;
	};
}
