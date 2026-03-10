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

#include "bone_manager.hpp"
#include "game/game_max_defines.hpp"

#include "resources/skin.hpp"
#include "resources/common_skin.hpp"
#include "world/world.hpp"

namespace SFG
{
	bone_manager::bone_manager()
	{
		_bone_batches	   = new pool_allocator<u16, u16, MAX_WORLD_BONE_BATCHES>();
		_local_matrices	   = new static_array<bone_batch, MAX_WORLD_BONE_BATCHES>();
		_inv_bind_matrices = new static_array<bone_batch, MAX_WORLD_BONE_BATCHES>();
		_abs_matrices	   = new static_array<bone_batch, MAX_WORLD_BONE_BATCHES>();
	}

	bone_manager::~bone_manager()
	{
		delete _bone_batches;
		delete _local_matrices;
		delete _inv_bind_matrices;
		delete _abs_matrices;
	}

	void bone_manager::init()
	{
	}

	void bone_manager::uninit()
	{
	}

	u16 bone_manager::allocate_batch(world& w, resource_handle skin_handle)
	{
		const u16 batch = _bone_batches->add();

		bone_batch& locals = _local_matrices->get(batch);
		bone_batch& abs	   = _abs_matrices->get(batch);
		bone_batch& inv	   = _inv_bind_matrices->get(batch);

		resource_manager&  rm			= w.get_resource_manager();
		chunk_allocator32& resource_aux = rm.get_aux();

		skin&				 sk			   = rm.get_resource<skin>(skin_handle);
		const chunk_handle32 joints_handle = sk.get_joints();
		const u16			 joints_count  = sk.get_joints_count();

		SFG_ASSERT(joints_count < MAX_WORLD_BONE_BATCH_SIZE);

		skin_joint* joints = resource_aux.get<skin_joint>(joints_handle);

		for (u16 i = 0; i < joints_count; i++)
		{
			skin_joint& j  = joints[i];
			locals.mats[i] = j.local_matrix;
			inv.mats[i]	   = j.inverse_bind_matrix;

			if (j.parent_index == -1)
				abs.mats[i] = j.local_matrix;
			else
				abs.mats[i] = abs.mats[j.parent_index] * j.local_matrix;

			const matrix4x3 bone_matrix = abs.mats[i] * inv.mats[i];
		}

		return batch;
	}

	void bone_manager::free_batch(u16 b)
	{
		_bone_batches->remove(b);
	}
}