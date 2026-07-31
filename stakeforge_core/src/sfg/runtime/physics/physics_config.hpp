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

#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/physics/physics_types.hpp>

namespace sfg
{
	struct physics_runtime_config_t
	{
		physics_runtime_config_t()
		{
			for (u32 i = 0; i < PHYSICS_COLLISION_LAYER_MAX; ++i)
				collision_masks[i] = UINT64_MAX;
		}

		vec3f_t gravity										 = {0.0f, -9.81f, 0.0f};
		u64		collision_masks[PHYSICS_COLLISION_LAYER_MAX] = {};
		u64		active_collision_layers						 = 1;
		u32		max_bodies									 = 16384;
		u32		body_mutex_count							 = 0;
		u32		max_body_pairs								 = 32768;
		u32		max_contact_constraints						 = 16384;
		u32		temp_allocator_bytes						 = 16 * 1024 * 1024;
		u32		body_reserve								 = 4096;
		u32		character_reserve							 = 128;
		u32		contact_event_reserve						 = 4096;
		u32		ragdoll_pose_budget_bytes					 = 4 * 1024 * 1024;
		u32		physics_rate								 = 100;
		u32		max_sub_steps								 = 4;
		bool	kinematic_sensors_collide_with_non_dynamic = false;
	};
}
