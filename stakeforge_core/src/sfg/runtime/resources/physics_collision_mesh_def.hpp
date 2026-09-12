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
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	struct physics_collision_mesh_def_t
	{
		vector_t<vec3f_t>		  vertices = {};
		vector_t<primitive_index> indices  = {};
	};

	SFG_DEFINE_TYPE_ID(physics_collision_mesh_def_t);

	struct physics_collision_mesh_def_reflection_t
	{
		physics_collision_mesh_def_reflection_t();
	};

	inline physics_collision_mesh_def_reflection_t g_reflect_physics_collision_mesh_def;
}
