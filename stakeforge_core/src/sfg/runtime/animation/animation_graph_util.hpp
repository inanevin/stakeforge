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

#include "animation_graph_types.hpp"

namespace sfg
{
	class chunk_allocator_t;
	struct skeleton_runtime_t;

	class animation_graph_util_t final
	{
	public:
		static chunk_handle32_t create_pose_from_skeleton(const skeleton_runtime_t& skeleton, const chunk_allocator_t& skeleton_memory, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory, chunk_allocator_t& aux_memory);
		static void				destroy_pose(chunk_handle32_t pose_handle, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory, chunk_allocator_t& aux_memory);
		static void				copy_pose(chunk_handle32_t source_pose_handle, chunk_handle32_t destination_pose_handle, chunk_allocator_t& pose_memory, chunk_allocator_t& pose_bone_memory);
		static void				advance_asm_state_phase(animation_graph_asm_state_t& state, f32 delta_time, f32 duration);
		static void				finalize_bones(const skeleton_runtime_t& skeleton, const chunk_allocator_t& skeleton_memory, chunk_handle32_t bones_handle, chunk_handle32_t inverse_binds_handle, chunk_allocator_t& bone_memory);
	};
}
