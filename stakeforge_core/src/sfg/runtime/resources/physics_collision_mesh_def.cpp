/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

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
