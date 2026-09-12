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

#include "animation_graph_util.hpp"
#include "animation_bone.hpp"

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

	void animation_graph_util_t::advance_asm_state_phase(animation_graph_asm_state_t& state, f32 delta_time, f32 duration)
	{
		if (duration <= 0.0f)
		{
			state._current_phase = 0.0f;
			return;
		}

		state._current_phase += delta_time / duration;
		state._current_phase = state.loop ? math::fmodf(state._current_phase, 1.0f) : math::min(state._current_phase, 1.0f);
	}

	void animation_graph_util_t::finalize_bones(const skeleton_runtime_t& skeleton, const chunk_allocator_t& skeleton_memory, chunk_handle32_t bones_handle, chunk_handle32_t inverse_binds_handle, chunk_allocator_t& bone_memory)
	{
		const skeleton_joint_runtime_t* joints			 = skeleton_memory.get<skeleton_joint_runtime_t>(skeleton.joints);
		const u32*						evaluation_order = skeleton_memory.get<u32>(skeleton.evaluation_order);
		animation_bone_t*				bones			 = bone_memory.get<animation_bone_t>(bones_handle);
		const animation_bone_t*			inverse_binds	 = bone_memory.get<animation_bone_t>(inverse_binds_handle);

		for (u32 i = 0; i < skeleton.joint_count; ++i)
		{
			const u32 joint_index  = evaluation_order[i];
			const u32 parent_index = joints[joint_index].parent_index;

			if (parent_index != SKELETON_JOINT_NO_PARENT)
				bones[joint_index].bone_transform = bones[parent_index].bone_transform * bones[joint_index].bone_transform;
		}

		for (u32 joint_index = 0; joint_index < skeleton.joint_count; ++joint_index)
			bones[joint_index].bone_transform = skeleton.skinning_transform * bones[joint_index].bone_transform * inverse_binds[joint_index].bone_transform;
	}
}
