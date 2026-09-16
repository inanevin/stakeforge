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
		bool	physics_enabled								 = false;
		bool	kinematic_sensors_collide_with_non_dynamic	 = false;
	};
}
