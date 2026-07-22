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
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

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

	struct alignas(32) animation_graph_param_t
	{
		union {
			f32		f32_value = 0.0f;
			vec2f_t vec2_value;
			vec3f_t vec3_value;
			quat_t	quat_value;
			bool	bool_value;
		};
		animation_param_type_e type = animation_param_type_e::f32;
	};

	struct animation_graph_mask_t
	{
		u64 bitmasks[2] = {};
	};

	struct animation_graph_clip_t
	{
		resource_handle_t clip = NULL_RESOURCE_HANDLE;
		union {
			f32		blend_value = 0.0f;
			vec2f_t blend_value_2d;
		};
	};

	struct animation_graph_asm_state_t
	{
		chunk_handle32_t				 clips			 = {};
		chunk_handle32_t				 blend_parameter = {};
		u32								 clip_count		 = 0;
		f32								 _duration		 = 0.0f;
		f32								 _current_time	 = 0.0f;
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
	};

	struct animation_graph_node_asm_t
	{
		chunk_handle32_t transitions			  = {};
		chunk_handle32_t first_state			  = {};
		chunk_handle32_t _current_state			  = {};
		chunk_handle32_t _current_transition	  = {};
		u32				 transition_count		  = 0;
		f32				 _current_transition_time = 0.0f;
	};

	struct animation_graph_node_bone_control_t
	{
	};

	struct animation_graph_node_ik_t
	{
	};

	struct alignas(64) animation_graph_node_t
	{
		union {
			animation_graph_node_asm_t			node_asm = {};
			animation_graph_node_bone_control_t node_bone_control;
			animation_graph_node_ik_t			node_ik;
		};
		chunk_handle32_t			next_node	= {};
		chunk_handle32_t			mask_handle = {};
		animation_graph_node_type_e type		= animation_graph_node_type_e::asm_node;
	};
}
