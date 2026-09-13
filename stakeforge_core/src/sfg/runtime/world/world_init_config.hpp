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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/physics/physics_config.hpp>
#include <sfg/runtime/world/world_debug_draw_config.hpp>

namespace sfg
{
	struct world_particle_simulation_config_t
	{
		vec3f_t gravity								  = {0.0f, -9.81f, 0.0f};
		f32		fixed_step_seconds					  = 1.0f / 60.0f;
		u32		emitter_initial_capacity			  = 32;
		u32		particle_per_emitter_initial_capacity = 256;
		u32		particle_max_count					  = 8192;
		u32		max_steps_per_tick					  = 4;
		u32		prewarm_max_steps					  = 240;
	};

	struct world_init_config_t
	{
		world_debug_draw_config_t		   debug_draw						 = {};
		physics_runtime_config_t		   physics							 = {};
		world_particle_simulation_config_t particle_simulation				 = {};
		vec2u16_t						   render_resolution				 = vec2u16_t(512, 512);
		u32								   render_entity_max_count			 = 256;
		u32								   render_sprite_max_count			 = 256;
		u32								   render_particle_max_count		 = 8192;
		u32								   render_bone_max_count			 = 256;
		u32								   render_bone_initial_capacity		 = 256;
		u32								   animation_processor_budget		 = 1 * 1024 * 1024;
		u32								   component_table_initial_capacity	 = 64;
		u32								   entity_free_list_initial_capacity = 1024;
		u32								   used_resource_initial_capacity	 = 512;
		u32								   text_allocation_initial_capacity	 = 1024;
		u32								   text_budget_bytes				 = 64 * 1024;
		bool							   physics_enabled					 = false;
	};
}
