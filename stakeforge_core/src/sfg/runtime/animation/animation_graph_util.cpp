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

#include "animation_graph_util.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/runtime/resources/skeleton.hpp>

namespace sfg
{
	chunk_handle32_t animation_graph_util_t::create_pose_from_skeleton(const skeleton_runtime_t& skeleton, const chunk_allocator_t& skeleton_memory, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory, chunk_allocator_t& aux_memory)
	{
		const skeleton_joint_runtime_t* skeleton_joints			  = skeleton_memory.get<skeleton_joint_runtime_t>(skeleton.joints);
		const u32*						skeleton_evaluation_order = skeleton_memory.get<u32>(skeleton.evaluation_order);
		const chunk_handle32_t			pose_handle				  = pose_memory.allocate_bytes(sizeof(animation_graph_pose_t), alignof(animation_graph_pose_t));
		animation_graph_pose_t*			pose					  = pose_memory.get<animation_graph_pose_t>(pose_handle);

		std::construct_at(pose, animation_graph_pose_t{});

		pose->bones		 = pose_bone_memory.allocate_bytes(sizeof(animation_graph_bone_t) * skeleton.joint_count, alignof(animation_graph_bone_t));
		pose->bone_count = skeleton.joint_count;

		animation_graph_bone_t* pose_bones			  = pose_bone_memory.get<animation_graph_bone_t>(pose->bones);
		u32*					pose_evaluation_order = nullptr;

		pose->evaluation_order = aux_memory.allocate<u32>(skeleton.joint_count, pose_evaluation_order);

		for (u32 bone_index = 0; bone_index < skeleton.joint_count; ++bone_index)
		{
			std::construct_at(&pose_bones[bone_index],
							  animation_graph_bone_t{
								  .local_matrix = skeleton_joints[bone_index].local,
								  .parent_index = skeleton_joints[bone_index].parent_index,
							  });
			pose_evaluation_order[bone_index] = skeleton_evaluation_order[bone_index];
		}

		return pose_handle;
	}

	void animation_graph_util_t::destroy_pose(chunk_handle32_t pose_handle, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory, chunk_allocator_t& aux_memory)
	{
		const animation_graph_pose_t& pose = *pose_memory.get<animation_graph_pose_t>(pose_handle);

		aux_memory.free(pose.evaluation_order);
		pose_bone_memory.free(pose.bones);
		pose_memory.free(pose_handle);
	}

	void animation_graph_util_t::copy_pose(chunk_handle32_t source_pose_handle, chunk_handle32_t destination_pose_handle, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory)
	{
		const animation_graph_pose_t& source_pose	   = *pose_memory.get<animation_graph_pose_t>(source_pose_handle);
		const animation_graph_pose_t& destination_pose = *pose_memory.get<animation_graph_pose_t>(destination_pose_handle);

		SFG_ASSERT(source_pose.bone_count == destination_pose.bone_count);

		const animation_graph_bone_t* source_bones		= pose_bone_memory.get<animation_graph_bone_t>(source_pose.bones);
		animation_graph_bone_t*		  destination_bones = pose_bone_memory.get<animation_graph_bone_t>(destination_pose.bones);

		SFG_MEMCPY(destination_bones, source_bones, sizeof(animation_graph_bone_t) * source_pose.bone_count);
	}

	void animation_graph_util_t::advance_asm_state_time(animation_graph_asm_state_t& state, f32 delta_time)
	{
		state._current_time += delta_time;

		if (state._duration > 0.0f)
			state._current_time = state.loop ? math::fmodf(state._current_time, state._duration) : math::min(state._current_time, state._duration);
		else
			state._current_time = 0.0f;
	}
}
