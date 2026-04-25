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

#include "common/size_definitions.hpp"
#include "math/matrix4x3.hpp"
#include "game/game_max_defines.hpp"
#include "resources/common_resources.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/static_array.hpp"

namespace SFG
{
	class world;
	class bone_manager
	{
	public:
		struct bone_batch
		{
			matrix4x3 mats[MAX_WORLD_BONE_BATCH_SIZE];
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bone_manager();
		~bone_manager();

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		u16	 allocate_batch(world& w, resource_handle skin);
		void free_batch(u16 b);

	private:
		sfg::pool_allocator_gen_t<u16, u16, MAX_WORLD_BONE_BATCHES>* _bone_batches		= nullptr;
		static_array<bone_batch, MAX_WORLD_BONE_BATCHES>*			 _local_matrices	= nullptr;
		static_array<bone_batch, MAX_WORLD_BONE_BATCHES>*			 _inv_bind_matrices = nullptr;
		static_array<bone_batch, MAX_WORLD_BONE_BATCHES>*			 _abs_matrices		= nullptr;
	};
}
