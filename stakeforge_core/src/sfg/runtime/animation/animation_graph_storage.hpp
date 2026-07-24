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

#include "animation_bone.hpp"
#include "animation_graph_types.hpp"

#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_allocator.hpp>

namespace sfg
{
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

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		animation_graph_storage_instance_t create_graph(const animation_graph_runtime_t& graph, const chunk_allocator_t& resource_memory, const skeleton_runtime_t& skeleton);
		void							   destroy_graph(const animation_graph_storage_instance_t& instance);
		void							   copy_pose_to_bones(chunk_handle32_t pose, span_t<animation_bone_t> bones) const;
		void							   process_graph(chunk_handle32_t nodes, u32 node_count, chunk_handle32_t initial_pose, const mat4x3_t& entity_transform, span_t<animation_bone_t> bones, f32 delta_time);

	private:
		void process_node_asm(animation_graph_node_asm_t& node, chunk_handle32_t mask_handle, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void process_node_bone_control(animation_graph_node_bone_control_t& node, const mat4x3_t& entity_transform, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void process_node_ik(animation_graph_node_ik_t& node, span_t<animation_graph_bone_t> pose_bones, f32 delta_time);
		void process_asm_state(animation_graph_asm_state_t& state, chunk_handle32_t mask_handle, f32 sample_time, span_t<animation_graph_bone_t> pose_bones);
		void sample_clip(resource_handle_t clip, f32 sample_time, const animation_graph_mask_t* mask, span_t<animation_graph_bone_t> pose_bones);

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
