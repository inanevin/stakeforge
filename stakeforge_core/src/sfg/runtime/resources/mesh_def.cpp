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

#include "mesh_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	primitive_static_def_reflection_t::primitive_static_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "primitive_static_def_t",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<vertex_static_t>(reflected_value_type_e::object, type_id_t<vertex_static_t>::value),
					 .name			= "vertices",
					 .display_name	= "Vertices",
					 .offset		= offsetof(primitive_static_def_t, vertices),
					 .size			= sizeof(vector_t<vertex_static_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_index>(reflected_value_type_e::u32),
					 .name			= "indices",
					 .display_name	= "Indices",
					 .offset		= offsetof(primitive_static_def_t, indices),
					 .size			= sizeof(vector_t<primitive_index>),
					 .type			= reflected_value_type_e::container},
					{.name = "material_index", .display_name = "Material Index", .offset = offsetof(primitive_static_def_t, material_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<primitive_static_def_t>::value,
			.size	   = sizeof(primitive_static_def_t),
			.alignment = alignof(primitive_static_def_t),
		});
	}

	primitive_skinned_def_reflection_t::primitive_skinned_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "primitive_skinned_def_t",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<vertex_skinned_t>(reflected_value_type_e::object, type_id_t<vertex_skinned_t>::value),
					 .name			= "vertices",
					 .display_name	= "Vertices",
					 .offset		= offsetof(primitive_skinned_def_t, vertices),
					 .size			= sizeof(vector_t<vertex_skinned_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_index>(reflected_value_type_e::u32),
					 .name			= "indices",
					 .display_name	= "Indices",
					 .offset		= offsetof(primitive_skinned_def_t, indices),
					 .size			= sizeof(vector_t<primitive_index>),
					 .type			= reflected_value_type_e::container},
					{.name = "material_index", .display_name = "Material Index", .offset = offsetof(primitive_skinned_def_t, material_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<primitive_skinned_def_t>::value,
			.size	   = sizeof(primitive_skinned_def_t),
			.alignment = alignof(primitive_skinned_def_t),
		});
	}

	mesh_def_reflection_t::mesh_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "mesh_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(mesh_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "local_bounds", .display_name = "Local Bounds", .sub_type_id = type_id_t<aabb_t>::value, .offset = offsetof(mesh_def_t, local_bounds), .size = sizeof(aabb_t), .type = reflected_value_type_e::object},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_static_def_t>(reflected_value_type_e::object, type_id_t<primitive_static_def_t>::value),
					 .name			= "static_primitives",
					 .display_name	= "Static Primitives",
					 .offset		= offsetof(mesh_def_t, static_primitives),
					 .size			= sizeof(vector_t<primitive_static_def_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_skinned_def_t>(reflected_value_type_e::object, type_id_t<primitive_skinned_def_t>::value),
					 .name			= "skinned_primitives",
					 .display_name	= "Skinned Primitives",
					 .offset		= offsetof(mesh_def_t, skinned_primitives),
					 .size			= sizeof(vector_t<primitive_skinned_def_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<mesh_def_t>::value,
			.size	   = sizeof(mesh_def_t),
			.alignment = alignof(mesh_def_t),
		});
	}
}
