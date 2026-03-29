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

#include "game/game_max_defines.hpp"
#include "animation_mask.hpp"
#include "world/world.hpp"
#include "resources/animation.hpp"
#include "resources/skin.hpp"
#include "resources/common_skin.hpp"

namespace SFG
{

	void animation_mask::mask_joints(world& w, resource_handle skin_handle, string_id* name_hashes, u16 name_hashes_count)
	{
		const resource_manager&	 rm		= w.get_resource_manager();
		const chunk_allocator32& rm_aux = rm.get_aux();
		const skin&				 sk		= w.get_resource_manager().get_resource<skin>(skin_handle);

		const skin_joint* joints_ptr = rm_aux.get<skin_joint>(sk.get_joints());

		for (u16 i = 0; i < sk.get_joints_count(); i++)
		{
			const skin_joint& j					 = joints_ptr[i];
			joints[j.model_node_index].is_masked = 1;
		}
	}

	void animation_mask::mask_joint(world& w, resource_handle skin_handle, string_id hash)
	{
		const resource_manager&	 rm		= w.get_resource_manager();
		const chunk_allocator32& rm_aux = rm.get_aux();
		const skin&				 sk		= w.get_resource_manager().get_resource<skin>(skin_handle);

		const skin_joint* joints_ptr = rm_aux.get<skin_joint>(sk.get_joints());

		for (u16 i = 0; i < sk.get_joints_count(); i++)
		{
			const skin_joint& j = joints_ptr[i];

			if (j.name_hash == hash)
			{
				joints[j.model_node_index].is_masked = 1;
				break;
			}
		}
	}

}
