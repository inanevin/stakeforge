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

#include "common_resources.hpp"

#include <sfg/runtime/animation/animation_graph_types.hpp>

namespace sfg
{
	struct animation_graph_resource_clip_t
	{
		resource_handle_t clip			 = NULL_RESOURCE_HANDLE;
		vec2f_t			  blend_value_2d = vec2f_t::zero;
		f32				  blend_value	 = 0.0f;
		f32				  playback_speed = 1.0f;
	};

	struct animation_graph_resource_state_t
	{
		chunk_handle32_t				 clips				   = {};
		chunk_handle32_t				 blend_triangles	   = {};
		u32								 clip_count			   = 0;
		u32								 blend_triangle_count  = 0;
		u32								 blend_parameter_index = UINT32_MAX;
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
		u32									  priority		   = 0;
		animation_graph_asm_transition_type_e type			   = animation_graph_asm_transition_type_e::equals;
		bool								  is_blended	   = true;
		bool								  is_interruptible = true;
		bool								  restart_target   = true;
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
		static constexpr u32 WIRE_VERSION = 5;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t animation_graph_resource_desc;
}
