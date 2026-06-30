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

#include "mesh_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry_v2.hpp>

#include <cstddef>

namespace sfg
{
	primitive_static_def_reflection_t::primitive_static_def_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "primitive_static_def_t",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<vertex_static_t>(reflected_value_type_e_v2::object, type_id_t<vertex_static_t>::value),
					 .name			= "vertices",
					 .display_name	= "Vertices",
					 .offset		= offsetof(primitive_static_def_t, vertices),
					 .size			= sizeof(vector_t<vertex_static_t>),
					 .type			= reflected_value_type_e_v2::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_index>(reflected_value_type_e_v2::u32),
					 .name			= "indices",
					 .display_name	= "Indices",
					 .offset		= offsetof(primitive_static_def_t, indices),
					 .size			= sizeof(vector_t<primitive_index>),
					 .type			= reflected_value_type_e_v2::container},
					{.name = "material_index", .display_name = "Material Index", .offset = offsetof(primitive_static_def_t, material_index), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
				},
			.type_id   = type_id_t<primitive_static_def_t>::value,
			.size	   = sizeof(primitive_static_def_t),
			.alignment = alignof(primitive_static_def_t),
		});
	}

	primitive_skinned_def_reflection_t::primitive_skinned_def_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "primitive_skinned_def_t",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<vertex_skinned_t>(reflected_value_type_e_v2::object, type_id_t<vertex_skinned_t>::value),
					 .name			= "vertices",
					 .display_name	= "Vertices",
					 .offset		= offsetof(primitive_skinned_def_t, vertices),
					 .size			= sizeof(vector_t<vertex_skinned_t>),
					 .type			= reflected_value_type_e_v2::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_index>(reflected_value_type_e_v2::u32),
					 .name			= "indices",
					 .display_name	= "Indices",
					 .offset		= offsetof(primitive_skinned_def_t, indices),
					 .size			= sizeof(vector_t<primitive_index>),
					 .type			= reflected_value_type_e_v2::container},
					{.name = "material_index", .display_name = "Material Index", .offset = offsetof(primitive_skinned_def_t, material_index), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
				},
			.type_id   = type_id_t<primitive_skinned_def_t>::value,
			.size	   = sizeof(primitive_skinned_def_t),
			.alignment = alignof(primitive_skinned_def_t),
		});
	}

	mesh_def_reflection_t::mesh_def_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "mesh_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(mesh_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e_v2::string},
					{.name = "local_bounds", .display_name = "Local Bounds", .sub_type_id = type_id_t<aabb_t>::value, .offset = offsetof(mesh_def_t, local_bounds), .size = sizeof(aabb_t), .type = reflected_value_type_e_v2::object},
					{.container_ops = reflection_container_ops_t::vector_ops<resource_handle_t>(reflected_value_type_e_v2::u64, REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID),
					 .name			= "materials",
					 .display_name	= "Materials",
					 .offset		= offsetof(mesh_def_t, materials),
					 .size			= sizeof(vector_t<resource_handle_t>),
					 .type			= reflected_value_type_e_v2::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_static_def_t>(reflected_value_type_e_v2::object, type_id_t<primitive_static_def_t>::value),
					 .name			= "static_primitives",
					 .display_name	= "Static Primitives",
					 .offset		= offsetof(mesh_def_t, static_primitives),
					 .size			= sizeof(vector_t<primitive_static_def_t>),
					 .type			= reflected_value_type_e_v2::container},
					{.container_ops = reflection_container_ops_t::vector_ops<primitive_skinned_def_t>(reflected_value_type_e_v2::object, type_id_t<primitive_skinned_def_t>::value),
					 .name			= "skinned_primitives",
					 .display_name	= "Skinned Primitives",
					 .offset		= offsetof(mesh_def_t, skinned_primitives),
					 .size			= sizeof(vector_t<primitive_skinned_def_t>),
					 .type			= reflected_value_type_e_v2::container},
				},
			.type_id   = type_id_t<mesh_def_t>::value,
			.size	   = sizeof(mesh_def_t),
			.alignment = alignof(mesh_def_t),
		});
	}
}
