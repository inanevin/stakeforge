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
