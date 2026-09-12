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

#include "vertex.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	vertex_static_reflection_t::vertex_static_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vertex_static_t",
			.fields =
				{
					{.name = "pos", .display_name = "Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(vertex_static_t, pos), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "normal", .display_name = "Normal", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(vertex_static_t, normal), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "tangent", .display_name = "Tangent", .sub_type_id = type_id_t<vec4f_t>::value, .offset = offsetof(vertex_static_t, tangent), .size = sizeof(vec4f_t), .type = reflected_value_type_e::object},
					{.name = "uv", .display_name = "UV", .sub_type_id = type_id_t<vec2f_t>::value, .offset = offsetof(vertex_static_t, uv), .size = sizeof(vec2f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<vertex_static_t>::value,
			.size	   = sizeof(vertex_static_t),
			.alignment = alignof(vertex_static_t),
		});
	}

	vertex_skinned_reflection_t::vertex_skinned_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vertex_skinned_t",
			.fields =
				{
					{.name = "pos", .display_name = "Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(vertex_skinned_t, pos), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "normal", .display_name = "Normal", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(vertex_skinned_t, normal), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "tangent", .display_name = "Tangent", .sub_type_id = type_id_t<vec4f_t>::value, .offset = offsetof(vertex_skinned_t, tangent), .size = sizeof(vec4f_t), .type = reflected_value_type_e::object},
					{.name = "uv", .display_name = "UV", .sub_type_id = type_id_t<vec2f_t>::value, .offset = offsetof(vertex_skinned_t, uv), .size = sizeof(vec2f_t), .type = reflected_value_type_e::object},
					{.name = "bone_weights", .display_name = "Bone Weights", .sub_type_id = type_id_t<vec4f_t>::value, .offset = offsetof(vertex_skinned_t, bone_weights), .size = sizeof(vec4f_t), .type = reflected_value_type_e::object},
					{.name = "bone_indices", .display_name = "Bone Indices", .sub_type_id = type_id_t<vec4u_t>::value, .offset = offsetof(vertex_skinned_t, bone_indices), .size = sizeof(vec4u_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<vertex_skinned_t>::value,
			.size	   = sizeof(vertex_skinned_t),
			.alignment = alignof(vertex_skinned_t),
		});
	}
}
