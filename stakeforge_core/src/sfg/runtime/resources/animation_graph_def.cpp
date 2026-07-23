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

#include "animation_graph_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

namespace sfg
{

	animation_graph_def_reflection_t::animation_graph_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "animation_graph_param_def_t",
			.display_name = "Parameter",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(animation_graph_param_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "id", .display_name = "ID", .offset = offsetof(animation_graph_param_def_t, id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<animation_param_type_e>::value, .offset = offsetof(animation_graph_param_def_t, type), .size = sizeof(animation_param_type_e), .type = reflected_value_type_e::u8},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 0, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "f32_value",
					 .display_name	= "Default",
					 .offset		= offsetof(animation_graph_param_def_t, f32_value),
					 .size			= sizeof(f32),
					 .type			= reflected_value_type_e::f32},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "vec2_value",
					 .display_name	= "Default",
					 .sub_type_id	= type_id_t<vec2f_t>::value,
					 .offset		= offsetof(animation_graph_param_def_t, vec2_value),
					 .size			= sizeof(vec2f_t),
					 .type			= reflected_value_type_e::object},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 2, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "vec3_value",
					 .display_name	= "Default",
					 .sub_type_id	= type_id_t<vec3f_t>::value,
					 .offset		= offsetof(animation_graph_param_def_t, vec3_value),
					 .size			= sizeof(vec3f_t),
					 .type			= reflected_value_type_e::object},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 3, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "quat_value",
					 .display_name	= "Default",
					 .sub_type_id	= type_id_t<quat_t>::value,
					 .offset		= offsetof(animation_graph_param_def_t, quat_value),
					 .size			= sizeof(quat_t),
					 .type			= reflected_value_type_e::object},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 5, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "bool_value",
					 .display_name	= "Default",
					 .offset		= offsetof(animation_graph_param_def_t, bool_value),
					 .size			= sizeof(bool),
					 .type			= reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<animation_graph_param_def_t>::value,
			.size	   = sizeof(animation_graph_param_def_t),
			.alignment = alignof(animation_graph_param_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_clip_def_t",
			.display_name = "Animation Clip",
			.fields =
				{
					{.name = "clip", .display_name = "Clip", .sub_type_id = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION, .offset = offsetof(animation_graph_clip_def_t, clip), .size = sizeof(resource_handle_t), .type = reflected_value_type_e::u64},
					{.name = "blend_value", .display_name = "Blend Value 1D", .offset = offsetof(animation_graph_clip_def_t, blend_value), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "blend_value_2d", .display_name = "Blend Value 2D", .sub_type_id = type_id_t<vec2f_t>::value, .offset = offsetof(animation_graph_clip_def_t, blend_value_2d), .size = sizeof(vec2f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_graph_clip_def_t>::value,
			.size	   = sizeof(animation_graph_clip_def_t),
			.alignment = alignof(animation_graph_clip_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_asm_state_def_t",
			.display_name = "State",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(animation_graph_asm_state_def_t, name),
						.size		  = sizeof(string_t),
						.type		  = reflected_value_type_e::string,
					},
					{
						.name		  = "id",
						.display_name = "ID",
						.offset		  = offsetof(animation_graph_asm_state_def_t, id),
						.size		  = sizeof(u32),
						.flags		  = reflected_field_flag_no_ui,
						.type		  = reflected_value_type_e::u32,
					},
					{
						.name		  = "editor_position",
						.display_name = "Editor Position",
						.sub_type_id  = type_id_t<vec2f_t>::value,
						.offset		  = offsetof(animation_graph_asm_state_def_t, editor_position),
						.size		  = sizeof(vec2f_t),
						.flags		  = reflected_field_flag_no_ui,
						.type		  = reflected_value_type_e::object,
					},
					{
						.name		  = "state_type",
						.display_name = "Blend Type",
						.sub_type_id  = type_id_t<animation_graph_asm_state_type_e>::value,
						.offset		  = offsetof(animation_graph_asm_state_def_t, state_type),
						.size		  = sizeof(animation_graph_asm_state_type_e),
						.type		  = reflected_value_type_e::u8,
					},
					{.name = "blend_parameter_id", .display_name = "Blend Parameter", .offset = offsetof(animation_graph_asm_state_def_t, blend_parameter_id), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "duration", .display_name = "Duration", .offset = offsetof(animation_graph_asm_state_def_t, duration), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "loop", .display_name = "Loop", .offset = offsetof(animation_graph_asm_state_def_t, loop), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_clip_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_clip_def_t>::value),
					 .name			= "clips",
					 .display_name	= "Clips",
					 .offset		= offsetof(animation_graph_asm_state_def_t, clips),
					 .size			= sizeof(vector_t<animation_graph_clip_def_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_graph_asm_state_def_t>::value,
			.size	   = sizeof(animation_graph_asm_state_def_t),
			.alignment = alignof(animation_graph_asm_state_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_asm_transition_def_t",
			.display_name = "Transition",
			.fields =
				{
					{.name = "id", .display_name = "ID", .offset = offsetof(animation_graph_asm_transition_def_t, id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "from_state_id", .display_name = "From State", .offset = offsetof(animation_graph_asm_transition_def_t, from_state_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "to_state_id", .display_name = "To State", .offset = offsetof(animation_graph_asm_transition_def_t, to_state_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "parameter_id", .display_name = "Parameter", .offset = offsetof(animation_graph_asm_transition_def_t, parameter_id), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name		   = "type",
					 .display_name = "Comparison",
					 .sub_type_id  = type_id_t<animation_graph_asm_transition_type_e>::value,
					 .offset	   = offsetof(animation_graph_asm_transition_def_t, type),
					 .size		   = sizeof(animation_graph_asm_transition_type_e),
					 .type		   = reflected_value_type_e::u8},
					{.name = "compare_value", .display_name = "Compare Value", .offset = offsetof(animation_graph_asm_transition_def_t, compare_value), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "duration", .display_name = "Duration", .offset = offsetof(animation_graph_asm_transition_def_t, duration), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "is_blended", .display_name = "Blend", .offset = offsetof(animation_graph_asm_transition_def_t, is_blended), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<animation_graph_asm_transition_def_t>::value,
			.size	   = sizeof(animation_graph_asm_transition_def_t),
			.alignment = alignof(animation_graph_asm_transition_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_node_asm_def_t",
			.display_name = "State Machine",
			.fields =
				{
					{.name = "first_state_id", .display_name = "First State", .offset = offsetof(animation_graph_node_asm_def_t, first_state_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.container_ops = reflection_container_ops_t::vector_ops<u32>(reflected_value_type_e::u32),
					 .name			= "masked_bones",
					 .display_name	= "Masked Bones",
					 .offset		= offsetof(animation_graph_node_asm_def_t, masked_bones),
					 .size			= sizeof(vector_t<u32>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_asm_state_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_asm_state_def_t>::value),
					 .name			= "states",
					 .display_name	= "States",
					 .offset		= offsetof(animation_graph_node_asm_def_t, states),
					 .size			= sizeof(vector_t<animation_graph_asm_state_def_t>),
					 .flags			= reflected_field_flag_no_ui,
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_asm_transition_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_asm_transition_def_t>::value),
					 .name			= "transitions",
					 .display_name	= "Transitions",
					 .offset		= offsetof(animation_graph_node_asm_def_t, transitions),
					 .size			= sizeof(vector_t<animation_graph_asm_transition_def_t>),
					 .flags			= reflected_field_flag_no_ui,
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_graph_node_asm_def_t>::value,
			.size	   = sizeof(animation_graph_node_asm_def_t),
			.alignment = alignof(animation_graph_node_asm_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_bone_control_entry_def_t",
			.display_name = "Bone Control",
			.fields =
				{
					{.name = "bone_index", .display_name = "Bone", .offset = offsetof(animation_graph_bone_control_entry_def_t, bone_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "parameter_id", .display_name = "Parameter ID", .offset = offsetof(animation_graph_bone_control_entry_def_t, parameter_id), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<animation_graph_bone_control_entry_def_t>::value,
			.size	   = sizeof(animation_graph_bone_control_entry_def_t),
			.alignment = alignof(animation_graph_bone_control_entry_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_node_bone_control_def_t",
			.display_name = "Bone Control",
			.fields =
				{
					{.name		   = "control_type",
					 .display_name = "Control Type",
					 .sub_type_id  = type_id_t<animation_graph_bone_control_type_e>::value,
					 .offset	   = offsetof(animation_graph_node_bone_control_def_t, control_type),
					 .size		   = sizeof(animation_graph_bone_control_type_e),
					 .type		   = reflected_value_type_e::u8},
					{.name		   = "control_space",
					 .display_name = "Control Space",
					 .sub_type_id  = type_id_t<animation_graph_bone_control_space_e>::value,
					 .offset	   = offsetof(animation_graph_node_bone_control_def_t, control_space),
					 .size		   = sizeof(animation_graph_bone_control_space_e),
					 .type		   = reflected_value_type_e::u8},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_bone_control_entry_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_bone_control_entry_def_t>::value),
					 .name			= "bones",
					 .display_name	= "Bones",
					 .offset		= offsetof(animation_graph_node_bone_control_def_t, bones),
					 .size			= sizeof(vector_t<animation_graph_bone_control_entry_def_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_graph_node_bone_control_def_t>::value,
			.size	   = sizeof(animation_graph_node_bone_control_def_t),
			.alignment = alignof(animation_graph_node_bone_control_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_node_ik_def_t",
			.display_name = "IK",
			.fields =
				{
					{.name = "reserved", .display_name = "Reserved", .offset = offsetof(animation_graph_node_ik_def_t, reserved), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<animation_graph_node_ik_def_t>::value,
			.size	   = sizeof(animation_graph_node_ik_def_t),
			.alignment = alignof(animation_graph_node_ik_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_node_def_t",
			.display_name = "Node",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(animation_graph_node_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "id", .display_name = "ID", .offset = offsetof(animation_graph_node_def_t, id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "next_node_id", .display_name = "Next Node", .offset = offsetof(animation_graph_node_def_t, next_node_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name		   = "editor_position",
					 .display_name = "Editor Position",
					 .sub_type_id  = type_id_t<vec2f_t>::value,
					 .offset	   = offsetof(animation_graph_node_def_t, editor_position),
					 .size		   = sizeof(vec2f_t),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::object},
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<animation_graph_node_type_e>::value, .offset = offsetof(animation_graph_node_def_t, type), .size = sizeof(animation_graph_node_type_e), .type = reflected_value_type_e::u8},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 0, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "asm_node",
					 .display_name	= "State Machine",
					 .sub_type_id	= type_id_t<animation_graph_node_asm_def_t>::value,
					 .offset		= offsetof(animation_graph_node_def_t, asm_node),
					 .size			= sizeof(animation_graph_node_asm_def_t),
					 .type			= reflected_value_type_e::object},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "bone_control_node",
					 .display_name	= "Bone Control",
					 .sub_type_id	= type_id_t<animation_graph_node_bone_control_def_t>::value,
					 .offset		= offsetof(animation_graph_node_def_t, bone_control_node),
					 .size			= sizeof(animation_graph_node_bone_control_def_t),
					 .type			= reflected_value_type_e::object},
					{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = 2, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "ik_node",
					 .display_name	= "IK",
					 .sub_type_id	= type_id_t<animation_graph_node_ik_def_t>::value,
					 .offset		= offsetof(animation_graph_node_def_t, ik_node),
					 .size			= sizeof(animation_graph_node_ik_def_t),
					 .type			= reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_graph_node_def_t>::value,
			.size	   = sizeof(animation_graph_node_def_t),
			.alignment = alignof(animation_graph_node_def_t),
		});

		registry.register_type({
			.name		  = "animation_graph_def_t",
			.display_name = "Animation Graph",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(animation_graph_def_t, name), .size = sizeof(string_t), .flags = reflected_field_flags_e::reflected_field_flag_no_ui, .type = reflected_value_type_e::string},
					{.name		   = "target_skeleton",
					 .display_name = "Target Skeleton",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
					 .offset	   = offsetof(animation_graph_def_t, target_skeleton),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
					{.name		   = "editor_view_offset",
					 .display_name = "Editor View Offset",
					 .sub_type_id  = type_id_t<vec2f_t>::value,
					 .offset	   = offsetof(animation_graph_def_t, editor_view_offset),
					 .size		   = sizeof(vec2f_t),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::object},
					{.name = "editor_view_zoom", .display_name = "Editor View Zoom", .offset = offsetof(animation_graph_def_t, editor_view_zoom), .size = sizeof(f32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::f32},
					{.name = "entry_node_id", .display_name = "Entry Node", .offset = offsetof(animation_graph_def_t, entry_node_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "output_node_id", .display_name = "Output Node", .offset = offsetof(animation_graph_def_t, output_node_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.name = "next_id", .display_name = "Next ID", .offset = offsetof(animation_graph_def_t, next_id), .size = sizeof(u32), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_param_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_param_def_t>::value),
					 .name			= "parameters",
					 .display_name	= "Parameters",
					 .offset		= offsetof(animation_graph_def_t, parameters),
					 .size			= sizeof(vector_t<animation_graph_param_def_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_graph_node_def_t>(reflected_value_type_e::object, type_id_t<animation_graph_node_def_t>::value),
					 .name			= "nodes",
					 .display_name	= "Nodes",
					 .offset		= offsetof(animation_graph_def_t, nodes),
					 .size			= sizeof(vector_t<animation_graph_node_def_t>),
					 .flags			= reflected_field_flag_no_ui,
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_graph_def_t>::value,
			.size	   = sizeof(animation_graph_def_t),
			.alignment = alignof(animation_graph_def_t),
		});
	}
}
