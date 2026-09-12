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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/common_resources.hpp>

namespace sfg
{
	enum class animation_graph_node_type_e : u8
	{
		asm_node,
		bone_controller,
		ik,
	};

	enum class animation_param_type_e : u8
	{
		f32,
		vec2,
		vec3,
		quat,
		boolean,
	};

	enum class animation_graph_asm_state_type_e : u8
	{
		no_blend,
		blend_1d,
		blend_2d,
	};

	enum class animation_graph_asm_transition_type_e : u8
	{
		equals,
		lequals,
		gequals,
		less,
		greater,
	};

	enum class animation_graph_bone_control_type_e : u8
	{
		rotation_override,
		rotation_additive,
		position_override,
		position_additive,
		look_at,
	};

	enum class animation_graph_bone_control_space_e : u8
	{
		local,
		component,
		world,
	};

	SFG_DEFINE_TYPE_ID(animation_graph_node_type_e);
	SFG_DEFINE_TYPE_ID(animation_param_type_e);
	SFG_DEFINE_TYPE_ID(animation_graph_asm_state_type_e);
	SFG_DEFINE_TYPE_ID(animation_graph_asm_transition_type_e);
	SFG_DEFINE_TYPE_ID(animation_graph_bone_control_type_e);
	SFG_DEFINE_TYPE_ID(animation_graph_bone_control_space_e);

	struct animation_graph_param_t
	{
		union {
			f32		f32_value = 0.0f;
			vec2f_t vec2_value;
			vec3f_t vec3_value;
			quat_t	quat_value;
			bool	bool_value;
		};
		animation_param_type_e type		 = animation_param_type_e::f32;
		sid_t				   name_hash = NULL_SID;
	};

	struct animation_graph_mask_t
	{
		u64 bitmasks[(MAX_SKELETON_BONES + 63) / 64] = {};
	};

	struct animation_graph_bone_t
	{
		mat4x3_t local_matrix = mat4x3_t::identity;
		u32		 parent_index = UINT32_MAX;
	};

	struct animation_graph_pose_t
	{
		chunk_handle32_t bones			  = {};
		chunk_handle32_t evaluation_order = {};
		u32				 bone_count		  = 0;
	};

	struct animation_graph_clip_t
	{
		resource_handle_t clip = NULL_RESOURCE_HANDLE;
		union {
			f32		blend_value = 0.0f;
			vec2f_t blend_value_2d;
		};
		f32 playback_speed = 1.0f;
	};

	struct animation_graph_blend_triangle_t
	{
		u16 clip_indices[3] = {};
	};

	struct animation_graph_asm_state_t
	{
		chunk_handle32_t				 clips				  = {};
		chunk_handle32_t				 blend_parameter	  = {};
		chunk_handle32_t				 blend_triangles	  = {};
		u32								 clip_count			  = 0;
		u32								 blend_triangle_count = 0;
		f32								 _current_phase		  = 0.0f;
		animation_graph_asm_state_type_e state_type			  = animation_graph_asm_state_type_e::no_blend;
		bool							 loop				  = false;
	};

	struct animation_graph_asm_transition_t
	{
		chunk_handle32_t					  from_state	   = {};
		chunk_handle32_t					  to_state		   = {};
		chunk_handle32_t					  parameter		   = {};
		f32									  compare_value	   = 0.0f;
		f32									  duration		   = 0.0f;
		u32									  priority		   = 0;
		animation_graph_asm_transition_type_e type			   = animation_graph_asm_transition_type_e::equals;
		bool								  is_blended	   = false;
		bool								  is_interruptible = true;
		bool								  restart_target   = true;
	};

	struct animation_graph_node_asm_t
	{
		chunk_handle32_t states					  = {};
		chunk_handle32_t transitions			  = {};
		chunk_handle32_t first_state			  = {};
		chunk_handle32_t _current_state			  = {};
		chunk_handle32_t _current_transition	  = {};
		u32				 state_count			  = 0;
		u32				 transition_count		  = 0;
		f32				 _current_transition_time = 0.0f;
	};

	struct animation_graph_node_bone_control_t
	{
		chunk_handle32_t					 bone_indices  = {};
		chunk_handle32_t					 parameters	   = {};
		u32									 bone_count	   = 0;
		animation_graph_bone_control_type_e	 control_type  = animation_graph_bone_control_type_e::rotation_override;
		animation_graph_bone_control_space_e control_space = animation_graph_bone_control_space_e::local;
	};

	struct animation_graph_node_ik_t
	{
	};

	struct animation_graph_node_t
	{
		union {
			animation_graph_node_asm_t			node_asm = {};
			animation_graph_node_bone_control_t node_bone_control;
			animation_graph_node_ik_t			node_ik;
		};
		chunk_handle32_t			pose_handle = {};
		chunk_handle32_t			mask_handle = {};
		animation_graph_node_type_e type		= animation_graph_node_type_e::asm_node;
	};

	struct animation_graph_types_reflection_t
	{
		animation_graph_types_reflection_t();
	};

	inline animation_graph_types_reflection_t g_reflect_animation_graph_types;
}
