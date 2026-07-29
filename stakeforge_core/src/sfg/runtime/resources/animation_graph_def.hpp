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

#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/animation/animation_graph_types.hpp>

namespace sfg
{
#define ANIMATION_GRAPH_DEF_NULL_ID UINT32_MAX

	struct animation_graph_param_def_t
	{
		string_t			   name		  = {};
		quat_t				   quat_value = quat_t::identity;
		vec3f_t				   vec3_value = vec3f_t::zero;
		vec2f_t				   vec2_value = vec2f_t::zero;
		f32					   f32_value  = 0.0f;
		animation_param_type_e type		  = animation_param_type_e::f32;
		bool				   bool_value = false;
	};

	struct animation_graph_clip_def_t
	{
		resource_handle_t clip			 = NULL_RESOURCE_HANDLE;
		vec2f_t			  blend_value_2d = vec2f_t::zero;
		f32				  blend_value	 = 0.0f;
		f32				  playback_speed = 1.0f;
	};

	struct animation_graph_asm_state_def_t
	{
		vector_t<animation_graph_clip_def_t> clips				= {};
		string_t							 name				= {};
		sid_t								 blend_parameter_id = NULL_SID;
		vec2f_t								 editor_position	= vec2f_t::zero;
		f32									 duration			= 0.0f;
		u32									 id					= ANIMATION_GRAPH_DEF_NULL_ID;
		animation_graph_asm_state_type_e	 state_type			= animation_graph_asm_state_type_e::no_blend;
		bool								 loop				= true;
	};

	struct animation_graph_asm_transition_def_t
	{
		sid_t								  parameter_id	= NULL_SID;
		f32									  compare_value = 0.0f;
		f32									  duration		= 0.0f;
		u32									  id			= ANIMATION_GRAPH_DEF_NULL_ID;
		u32									  from_state_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32									  to_state_id	= ANIMATION_GRAPH_DEF_NULL_ID;
		animation_graph_asm_transition_type_e type			= animation_graph_asm_transition_type_e::equals;
		bool								  is_blended	= true;
	};

	struct animation_graph_node_asm_def_t
	{
		vector_t<animation_graph_asm_state_def_t>	   states		  = {};
		vector_t<animation_graph_asm_transition_def_t> transitions	  = {};
		vector_t<u32>								   masked_bones	  = {};
		u32											   first_state_id = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct animation_graph_bone_control_entry_def_t
	{
		sid_t parameter_id = NULL_SID;
		u32	  bone_index   = UINT32_MAX;
	};

	struct animation_graph_node_bone_control_def_t
	{
		vector_t<animation_graph_bone_control_entry_def_t> bones		 = {};
		animation_graph_bone_control_type_e				   control_type	 = animation_graph_bone_control_type_e::rotation_override;
		animation_graph_bone_control_space_e			   control_space = animation_graph_bone_control_space_e::local;
	};

	struct animation_graph_node_ik_def_t
	{
		u32 reserved = 0;
	};

	struct animation_graph_node_def_t
	{
		animation_graph_node_asm_def_t			asm_node		  = {};
		animation_graph_node_bone_control_def_t bone_control_node = {};
		animation_graph_node_ik_def_t			ik_node			  = {};
		string_t								name			  = {};
		vec2f_t									editor_position	  = vec2f_t::zero;
		u32										id				  = ANIMATION_GRAPH_DEF_NULL_ID;
		u32										next_node_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		animation_graph_node_type_e				type			  = animation_graph_node_type_e::asm_node;
	};

	struct animation_graph_def_t
	{
		vector_t<animation_graph_param_def_t> parameters		 = {};
		vector_t<animation_graph_node_def_t>  nodes				 = {};
		string_t							  name				 = {};
		resource_handle_t					  target_skeleton	 = NULL_RESOURCE_HANDLE;
		vec2f_t								  editor_view_offset = vec2f_t::zero;
		f32									  editor_view_zoom	 = 1.0f;
		u32									  entry_node_id		 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32									  output_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32									  next_id			 = 1;

		animation_graph_node_def_t*		  find_node(u32 node_id);
		const animation_graph_node_def_t* find_node(u32 node_id) const;
	};

	SFG_DEFINE_TYPE_ID(animation_graph_param_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_clip_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_asm_state_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_asm_transition_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_node_asm_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_bone_control_entry_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_node_bone_control_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_node_ik_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_node_def_t);
	SFG_DEFINE_TYPE_ID(animation_graph_def_t);

	struct animation_graph_def_reflection_t
	{
		animation_graph_def_reflection_t();
	};

	inline animation_graph_def_reflection_t g_reflect_animation_graph_def;
}
