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

	struct animation_graph_asm_state_t
	{
		chunk_handle32_t				 clips			 = {};
		chunk_handle32_t				 blend_parameter = {};
		u32								 clip_count		 = 0;
		f32								 _current_phase	 = 0.0f;
		animation_graph_asm_state_type_e state_type		 = animation_graph_asm_state_type_e::no_blend;
		bool							 loop			 = false;
	};

	struct animation_graph_asm_transition_t
	{
		chunk_handle32_t					  from_state	= {};
		chunk_handle32_t					  to_state		= {};
		chunk_handle32_t					  parameter		= {};
		f32									  compare_value = 0.0f;
		f32									  duration		= 0.0f;
		animation_graph_asm_transition_type_e type			= animation_graph_asm_transition_type_e::equals;
		bool								  is_blended	= false;
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
