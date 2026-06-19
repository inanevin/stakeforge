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

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	primitive_static_def_reflection_t::primitive_static_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<primitive_static_def_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name			= "vertices",
			 .display_name	= "Vertices",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= type_id_t<vertex_static_t>::value,
			 .container_ops = reflected_vector_ops<vertex_static_t>(),
			 .offset		= offsetof(primitive_static_def_t, vertices),
			 .size			= sizeof(vector_t<vertex_static_t>)},
			{.name			= "indices",
			 .display_name	= "Indices",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= "u32"_hs,
			 .container_ops = reflected_vector_ops<primitive_index>(),
			 .offset		= offsetof(primitive_static_def_t, indices),
			 .size			= sizeof(vector_t<primitive_index>)},
			{.name = "material_index", .display_name = "Material Index", .type = reflected_value_type_e::u32, .offset = offsetof(primitive_static_def_t, material_index), .size = sizeof(u32)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "primitive_static_def_t",
			.type_id   = type_id_t<primitive_static_def_t>::value,
			.size	   = sizeof(primitive_static_def_t),
			.alignment = alignof(primitive_static_def_t),
		});
	}

	primitive_skinned_def_reflection_t::primitive_skinned_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<primitive_skinned_def_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name			= "vertices",
			 .display_name	= "Vertices",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= type_id_t<vertex_skinned_t>::value,
			 .container_ops = reflected_vector_ops<vertex_skinned_t>(),
			 .offset		= offsetof(primitive_skinned_def_t, vertices),
			 .size			= sizeof(vector_t<vertex_skinned_t>)},
			{.name			= "indices",
			 .display_name	= "Indices",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= "u32"_hs,
			 .container_ops = reflected_vector_ops<primitive_index>(),
			 .offset		= offsetof(primitive_skinned_def_t, indices),
			 .size			= sizeof(vector_t<primitive_index>)},
			{.name = "material_index", .display_name = "Material Index", .type = reflected_value_type_e::u32, .offset = offsetof(primitive_skinned_def_t, material_index), .size = sizeof(u32)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "primitive_skinned_def_t",
			.type_id   = type_id_t<primitive_skinned_def_t>::value,
			.size	   = sizeof(primitive_skinned_def_t),
			.alignment = alignof(primitive_skinned_def_t),
		});
	}

	mesh_def_reflection_t::mesh_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<mesh_def_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "name", .display_name = "Name", .type = reflected_value_type_e::string, .offset = offsetof(mesh_def_t, name), .size = sizeof(string_t)},
			{.name = "local_bounds", .display_name = "Local Bounds", .type = reflected_value_type_e::object, .value_type_id = type_id_t<aabb_t>::value, .offset = offsetof(mesh_def_t, local_bounds), .size = sizeof(aabb_t)},
			{.name			= "materials",
			 .display_name	= "Materials",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= "material_handle"_hs,
			 .container_ops = reflected_vector_ops<resource_handle_t>(),
			 .offset		= offsetof(mesh_def_t, materials),
			 .size			= sizeof(vector_t<resource_handle_t>)},
			{.name			= "static_primitives",
			 .display_name	= "Static Primitives",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= type_id_t<primitive_static_def_t>::value,
			 .container_ops = reflected_vector_ops<primitive_static_def_t>(),
			 .offset		= offsetof(mesh_def_t, static_primitives),
			 .size			= sizeof(vector_t<primitive_static_def_t>)},
			{.name			= "skinned_primitives",
			 .display_name	= "Skinned Primitives",
			 .type			= reflected_value_type_e::vector,
			 .sub_type_id	= type_id_t<primitive_skinned_def_t>::value,
			 .container_ops = reflected_vector_ops<primitive_skinned_def_t>(),
			 .offset		= offsetof(mesh_def_t, skinned_primitives),
			 .size			= sizeof(vector_t<primitive_skinned_def_t>)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "mesh_def_t",
			.type_id   = type_id_t<mesh_def_t>::value,
			.size	   = sizeof(mesh_def_t),
			.alignment = alignof(mesh_def_t),
		});
	}
}
