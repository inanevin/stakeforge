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

#include "physics_collision_mesh_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	physics_collision_mesh_def_reflection_t::physics_collision_mesh_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "physics_collision_mesh_def_t",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<vec3f_t>(reflected_value_type_e::object, type_id_t<vec3f_t>::value),
					 .name			= "vertices",
					 .display_name	= "Vertices",
					 .offset		= offsetof(physics_collision_mesh_def_t, vertices),
					 .size			= sizeof(vector_t<vec3f_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_index>(reflected_value_type_e::u32),
					 .name			= "indices",
					 .display_name	= "Indices",
					 .offset		= offsetof(physics_collision_mesh_def_t, indices),
					 .size			= sizeof(vector_t<primitive_index>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<physics_collision_mesh_def_t>::value,
			.size	   = sizeof(physics_collision_mesh_def_t),
			.alignment = alignof(physics_collision_mesh_def_t),
		});
	}
}
