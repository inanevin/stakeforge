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

#include "common_resources.hpp"

#include <sfg/runtime/animation/animation_graph_types.hpp>

namespace sfg
{
	struct animation_graph_resource_param_t
	{
		animation_graph_param_t value	  = {};
		sid_t					name_hash = NULL_SID;
	};

	struct animation_graph_resource_clip_t
	{
		resource_handle_t clip			 = NULL_RESOURCE_HANDLE;
		vec2f_t			  blend_value_2d = vec2f_t::zero;
		f32				  blend_value	 = 0.0f;
	};

	struct animation_graph_resource_state_t
	{
		chunk_handle32_t				 clips				   = {};
		u32								 clip_count			   = 0;
		u32								 blend_parameter_index = UINT32_MAX;
		f32								 duration			   = 0.0f;
		animation_graph_asm_state_type_e state_type			   = animation_graph_asm_state_type_e::no_blend;
		bool							 loop				   = false;
	};

	struct animation_graph_resource_transition_t
	{
		u32									  from_state_index = UINT32_MAX;
		u32									  to_state_index   = UINT32_MAX;
		u32									  parameter_index  = UINT32_MAX;
		f32									  compare_value	   = 0.0f;
		f32									  duration		   = 0.0f;
		animation_graph_asm_transition_type_e type			   = animation_graph_asm_transition_type_e::equals;
		bool								  is_blended	   = true;
	};

	struct animation_graph_resource_asm_t
	{
		chunk_handle32_t	   states			 = {};
		chunk_handle32_t	   transitions		 = {};
		animation_graph_mask_t mask				 = {};
		u32					   state_count		 = 0;
		u32					   transition_count	 = 0;
		u32					   first_state_index = UINT32_MAX;
	};

	struct animation_graph_resource_bone_control_entry_t
	{
		u32 bone_index		= UINT32_MAX;
		u32 parameter_index = UINT32_MAX;
	};

	struct animation_graph_resource_bone_control_t
	{
		chunk_handle32_t					 bones		   = {};
		u32									 bone_count	   = 0;
		animation_graph_bone_control_type_e	 control_type  = animation_graph_bone_control_type_e::rotation_override;
		animation_graph_bone_control_space_e control_space = animation_graph_bone_control_space_e::local;
	};

	struct animation_graph_resource_node_t
	{
		animation_graph_resource_asm_t			asm_node		  = {};
		animation_graph_resource_bone_control_t bone_control_node = {};
		animation_graph_node_type_e				type			  = animation_graph_node_type_e::asm_node;
	};

	struct animation_graph_runtime_t
	{
		chunk_handle32_t  parameters	  = {};
		chunk_handle32_t  nodes			  = {};
		resource_handle_t target_skeleton = NULL_RESOURCE_HANDLE;
		u32				  parameter_count = 0;
		u32				  node_count	  = 0;
	};

	struct animation_graph_internals_t
	{
		u32 reserved = 0;
	};

	class animation_graph_loader_t final
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('A', 'G', 'R', 'F');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t animation_graph_resource_desc;
}
