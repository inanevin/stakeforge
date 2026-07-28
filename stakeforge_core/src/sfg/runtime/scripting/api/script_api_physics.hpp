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

#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct character_mover_state_t;
	struct physics_body_state_t;
	struct physics_hit_t;
	struct physics_linecast_t;
	struct physics_query_filter_t;
	struct physics_query_result_t;
	struct physics_raycast_t;
	struct physics_spherecast_t;
	struct vec3f_t;
	class world_t;

	u8	 api_physics_set_body_linear_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity);
	u8	 api_physics_set_body_angular_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity);
	u8	 api_physics_add_body_force(world_t* world, entity_id_t entity, const vec3f_t* force);
	u8	 api_physics_add_body_impulse(world_t* world, entity_id_t entity, const vec3f_t* impulse);
	u8	 api_physics_wake_body(world_t* world, entity_id_t entity);
	u8	 api_physics_get_body_state(const world_t* world, entity_id_t entity, physics_body_state_t* out_state);
	u8	 api_physics_raycast_any(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter);
	u8	 api_physics_raycast_closest(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hit);
	u8	 api_physics_raycast_all(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result);
	u8	 api_physics_linecast_closest(const world_t* world, const physics_linecast_t* line, const physics_query_filter_t* filter, physics_hit_t* out_hit);
	void api_physics_linecast_closest_batch(const world_t* world, const physics_linecast_t* lines, physics_hit_t* out_hits, u32 count, const physics_query_filter_t* filter);
	u8	 api_physics_spherecast_any(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter);
	u8	 api_physics_spherecast_closest(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hit);
	u8	 api_physics_spherecast_all(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result);
	u8	 api_physics_set_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity);
	u8	 api_physics_add_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity);
	u8	 api_physics_jump_character(world_t* world, entity_id_t entity, f32 speed);
	u8	 api_physics_teleport_character(world_t* world, entity_id_t entity, const vec3f_t* position);
	u8	 api_physics_get_character_state(const world_t* world, entity_id_t entity, character_mover_state_t* out_state);

	struct script_api_physics_t
	{
		u32												 size					   = 0;
		u32												 version				   = 0;
		decltype(&api_physics_set_body_linear_velocity)	 set_body_linear_velocity  = nullptr;
		decltype(&api_physics_set_body_angular_velocity) set_body_angular_velocity = nullptr;
		decltype(&api_physics_add_body_force)			 add_body_force			   = nullptr;
		decltype(&api_physics_add_body_impulse)			 add_body_impulse		   = nullptr;
		decltype(&api_physics_wake_body)				 wake_body				   = nullptr;
		decltype(&api_physics_get_body_state)			 get_body_state			   = nullptr;
		decltype(&api_physics_raycast_any)				 raycast_any			   = nullptr;
		decltype(&api_physics_raycast_closest)			 raycast_closest		   = nullptr;
		decltype(&api_physics_raycast_all)				 raycast_all			   = nullptr;
		decltype(&api_physics_linecast_closest)			 linecast_closest		   = nullptr;
		decltype(&api_physics_linecast_closest_batch)	 linecast_closest_batch	   = nullptr;
		decltype(&api_physics_spherecast_any)			 spherecast_any			   = nullptr;
		decltype(&api_physics_spherecast_closest)		 spherecast_closest		   = nullptr;
		decltype(&api_physics_spherecast_all)			 spherecast_all			   = nullptr;
		decltype(&api_physics_set_character_velocity)	 set_character_velocity	   = nullptr;
		decltype(&api_physics_add_character_velocity)	 add_character_velocity	   = nullptr;
		decltype(&api_physics_jump_character)			 jump_character			   = nullptr;
		decltype(&api_physics_teleport_character)		 teleport_character		   = nullptr;
		decltype(&api_physics_get_character_state)		 get_character_state	   = nullptr;
	};

	const script_api_physics_t& get_script_api_physics();
}
