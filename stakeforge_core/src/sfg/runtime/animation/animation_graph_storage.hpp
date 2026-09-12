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

#include "animation_bone.hpp"
#include "animation_graph_types.hpp"

#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_allocator.hpp>

namespace sfg
{
	struct animation_runtime_t;
	struct animation_graph_runtime_t;
	struct skeleton_runtime_t;

	struct animation_graph_storage_instance_t
	{
		chunk_handle32_t initial_pose	 = {};
		chunk_handle32_t parameters		 = {};
		chunk_handle32_t nodes			 = {};
		u32				 parameter_count = 0;
		u32				 node_count		 = 0;
	};

	class animation_graph_storage_t final
	{
	public:
		animation_graph_storage_t() = default;
		~animation_graph_storage_t();
		animation_graph_storage_t(const animation_graph_storage_t&)			   = delete;
		animation_graph_storage_t& operator=(const animation_graph_storage_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 storage_memory);
		void uninit();
		void reset();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		animation_graph_storage_instance_t create_graph(const animation_graph_runtime_t& graph, const chunk_allocator_t& resource_memory, const skeleton_runtime_t& skeleton);
		void							   destroy_graph(const animation_graph_storage_instance_t& instance);
		void							   copy_pose_to_bones(chunk_handle32_t pose, span_t<animation_bone_t> bones) const;
		void							   advance_graph(chunk_handle32_t nodes, u32 node_count, f32 delta_time);
		void							   process_graph(chunk_handle32_t nodes, u32 node_count, chunk_handle32_t initial_pose, const mat4x3_t& entity_transform, span_t<animation_bone_t> bones, f32 delta_time);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		animation_graph_param_t*	   find_parameter(chunk_handle32_t parameters, u32 parameter_count, sid_t parameter_hash);
		const animation_graph_param_t* find_parameter(chunk_handle32_t parameters, u32 parameter_count, sid_t parameter_hash) const;

	private:
		struct asm_update_t
		{
			animation_graph_asm_state_t* current_state	   = nullptr;
			animation_graph_asm_state_t* transition_target = nullptr;
			f32							 transition_blend  = 0.0f;
		};

		asm_update_t update_node_asm(animation_graph_node_asm_t& node, f32 delta_time);
		void		 advance_node_asm(animation_graph_node_asm_t& node, f32 delta_time);
		void		 advance_asm_state(animation_graph_asm_state_t& state, f32 delta_time);
		bool		 compute_asm_state_blend_weights(const animation_graph_asm_state_t& state, span_t<f32> blend_weights) const;
		bool		 is_asm_transition_eligible(const animation_graph_asm_transition_t& transition) const;
		void		 process_node_asm(animation_graph_node_asm_t& node, chunk_handle32_t mask_handle, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void		 process_node_bone_control(animation_graph_node_bone_control_t& node, const mat4x3_t& entity_transform, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void		 process_node_ik(animation_graph_node_ik_t& node, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void		 process_asm_state(animation_graph_asm_state_t& state, chunk_handle32_t mask_handle, f32 delta_time, span_t<animation_graph_bone_t> pose_bones);
		void		 sample_clip(const animation_runtime_t& animation, f32 sample_time, const animation_graph_mask_t* mask, span_t<animation_graph_bone_t> pose_bones);

	private:
		chunk_allocator32_t _nodes			 = {};
		chunk_allocator32_t _params			 = {};
		chunk_allocator32_t _masks			 = {};
		chunk_allocator32_t _clips			 = {};
		chunk_allocator32_t _asm_states		 = {};
		chunk_allocator32_t _asm_transitions = {};
		chunk_allocator32_t _poses			 = {};
		chunk_allocator32_t _pose_bones		 = {};
		chunk_allocator32_t _aux			 = {};
	};
}
