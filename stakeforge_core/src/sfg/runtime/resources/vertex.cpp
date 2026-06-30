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
