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
		u32								   animation_graph_budget_bytes		 = 1 * 1024 * 1024;
		u32								   component_table_initial_capacity	 = 64;
		u32								   entity_free_list_initial_capacity = 1024;
		u32								   used_resource_initial_capacity	 = 512;
		u32								   text_allocation_initial_capacity	 = 1024;
		u32								   text_budget_bytes				 = 64 * 1024;
		bool							   physics_enabled					 = false;
	};
}
