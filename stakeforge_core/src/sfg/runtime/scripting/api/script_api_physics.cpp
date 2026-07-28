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

#include "script_api_physics.hpp"

namespace sfg
{
	u8 api_physics_set_body_linear_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		return 0;
	}

	u8 api_physics_set_body_angular_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		return 0;
	}

	u8 api_physics_add_body_force(world_t* world, entity_id_t entity, const vec3f_t* force)
	{
		return 0;
	}

	u8 api_physics_add_body_impulse(world_t* world, entity_id_t entity, const vec3f_t* impulse)
	{
		return 0;
	}

	u8 api_physics_wake_body(world_t* world, entity_id_t entity)
	{
		return 0;
	}

	u8 api_physics_get_body_state(const world_t* world, entity_id_t entity, physics_body_state_t* out_state)
	{
		return 0;
	}

	u8 api_physics_raycast_any(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter)
	{
		return 0;
	}

	u8 api_physics_raycast_closest(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		return 0;
	}

	u8 api_physics_raycast_all(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result)
	{
		return 0;
	}

	u8 api_physics_linecast_closest(const world_t* world, const physics_linecast_t* line, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		return 0;
	}

	void api_physics_linecast_closest_batch(const world_t* world, const physics_linecast_t* lines, physics_hit_t* out_hits, u32 count, const physics_query_filter_t* filter)
	{
	}

	u8 api_physics_spherecast_any(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter)
	{
		return 0;
	}

	u8 api_physics_spherecast_closest(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		return 0;
	}

	u8 api_physics_spherecast_all(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result)
	{
		return 0;
	}

	u8 api_physics_set_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		return 0;
	}

	u8 api_physics_add_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		return 0;
	}

	u8 api_physics_jump_character(world_t* world, entity_id_t entity, f32 speed)
	{
		return 0;
	}

	u8 api_physics_teleport_character(world_t* world, entity_id_t entity, const vec3f_t* position)
	{
		return 0;
	}

	u8 api_physics_get_character_state(const world_t* world, entity_id_t entity, character_mover_state_t* out_state)
	{
		return 0;
	}

	const script_api_physics_t& get_script_api_physics()
	{
		static const script_api_physics_t api{
			.size					   = static_cast<u32>(sizeof(script_api_physics_t)),
			.version				   = 1,
			.set_body_linear_velocity  = api_physics_set_body_linear_velocity,
			.set_body_angular_velocity = api_physics_set_body_angular_velocity,
			.add_body_force			   = api_physics_add_body_force,
			.add_body_impulse		   = api_physics_add_body_impulse,
			.wake_body				   = api_physics_wake_body,
			.get_body_state			   = api_physics_get_body_state,
			.raycast_any			   = api_physics_raycast_any,
			.raycast_closest		   = api_physics_raycast_closest,
			.raycast_all			   = api_physics_raycast_all,
			.linecast_closest		   = api_physics_linecast_closest,
			.linecast_closest_batch	   = api_physics_linecast_closest_batch,
			.spherecast_any			   = api_physics_spherecast_any,
			.spherecast_closest		   = api_physics_spherecast_closest,
			.spherecast_all			   = api_physics_spherecast_all,
			.set_character_velocity	   = api_physics_set_character_velocity,
			.add_character_velocity	   = api_physics_add_character_velocity,
			.jump_character			   = api_physics_jump_character,
			.teleport_character		   = api_physics_teleport_character,
			.get_character_state	   = api_physics_get_character_state,
		};

		return api;
	}
}
