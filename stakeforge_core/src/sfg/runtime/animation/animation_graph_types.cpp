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

#include "animation_graph_types.hpp"

#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	animation_graph_types_reflection_t::animation_graph_types_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_graph_node_type_e",
			.fields =
				{
					{.name = "asm_node", .display_name = "State Machine"},
					{.name = "bone_controller", .display_name = "Bone Control"},
					{.name = "ik", .display_name = "IK"},
				},
			.type_id   = type_id_t<animation_graph_node_type_e>::value,
			.size	   = sizeof(animation_graph_node_type_e),
			.alignment = alignof(animation_graph_node_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "animation_param_type_e",
			.fields =
				{
					{.name = "f32", .display_name = "Float"},
					{.name = "vec2", .display_name = "Vector 2"},
					{.name = "vec3", .display_name = "Vector 3"},
					{.name = "quat", .display_name = "Rotation"},
					{.name = "boolean", .display_name = "Boolean"},
				},
			.type_id   = type_id_t<animation_param_type_e>::value,
			.size	   = sizeof(animation_param_type_e),
			.alignment = alignof(animation_param_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "animation_graph_asm_state_type_e",
			.fields =
				{
					{.name = "no_blend", .display_name = "No Blend"},
					{.name = "blend_1d", .display_name = "Blend 1D"},
					{.name = "blend_2d", .display_name = "Blend 2D"},
				},
			.type_id   = type_id_t<animation_graph_asm_state_type_e>::value,
			.size	   = sizeof(animation_graph_asm_state_type_e),
			.alignment = alignof(animation_graph_asm_state_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "animation_graph_asm_transition_type_e",
			.fields =
				{
					{.name = "equals", .display_name = "Equals"},
					{.name = "lequals", .display_name = "Less Or Equal"},
					{.name = "gequals", .display_name = "Greater Or Equal"},
					{.name = "less", .display_name = "Less"},
					{.name = "greater", .display_name = "Greater"},
				},
			.type_id   = type_id_t<animation_graph_asm_transition_type_e>::value,
			.size	   = sizeof(animation_graph_asm_transition_type_e),
			.alignment = alignof(animation_graph_asm_transition_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "animation_graph_bone_control_type_e",
			.fields =
				{
					{.name = "rotation_override", .display_name = "Rotation Override"},
					{.name = "rotation_additive", .display_name = "Rotation Additive"},
					{.name = "position_override", .display_name = "Position Override"},
					{.name = "position_additive", .display_name = "Position Additive"},
					{.name = "look_at", .display_name = "Look At"},
				},
			.type_id   = type_id_t<animation_graph_bone_control_type_e>::value,
			.size	   = sizeof(animation_graph_bone_control_type_e),
			.alignment = alignof(animation_graph_bone_control_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "animation_graph_bone_control_space_e",
			.fields =
				{
					{.name = "local", .display_name = "Local"},
					{.name = "component", .display_name = "Component"},
					{.name = "world", .display_name = "World"},
				},
			.type_id   = type_id_t<animation_graph_bone_control_space_e>::value,
			.size	   = sizeof(animation_graph_bone_control_space_e),
			.alignment = alignof(animation_graph_bone_control_space_e),
			.flags	   = reflected_type_flag_enum,
		});
	}
}
