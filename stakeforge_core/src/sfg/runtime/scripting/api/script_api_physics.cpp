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

#include <sfg/io/assert.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	u8 api_physics_set_body_linear_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(velocity != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		physics.set_body_linear_velocity(entity, *velocity);
		return 1;
	}

	u8 api_physics_set_body_angular_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(velocity != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		physics.set_body_angular_velocity(entity, *velocity);
		return 1;
	}

	u8 api_physics_add_body_force(world_t* world, entity_id_t entity, const vec3f_t* force)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(force != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		physics.add_body_force(entity, *force);
		return 1;
	}

	u8 api_physics_add_body_impulse(world_t* world, entity_id_t entity, const vec3f_t* impulse)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(impulse != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		physics.add_body_impulse(entity, *impulse);
		return 1;
	}

	u8 api_physics_wake_body(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		physics.wake_body(entity);
		return 1;
	}

	u8 api_physics_get_body_state(const world_t* world, entity_id_t entity, physics_body_state_t* out_state)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_state != nullptr);

		*out_state					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_body(entity))
			return 0;

		return physics.get_body_state(entity, *out_state) ? 1 : 0;
	}

	u8 api_physics_raycast_any(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(ray != nullptr);

		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		return physics.raycast_any(*ray, query_filter) ? 1 : 0;
	}

	u8 api_physics_raycast_closest(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(ray != nullptr);
		SFG_ASSERT(out_hit != nullptr);

		*out_hit					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		return physics.raycast_closest(*ray, *out_hit, query_filter) ? 1 : 0;
	}

	u8 api_physics_raycast_all(const world_t* world, const physics_raycast_t* ray, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(ray != nullptr);
		SFG_ASSERT(out_hits != nullptr || capacity == 0);
		SFG_ASSERT(out_result != nullptr);

		*out_result					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		*out_result								  = physics.raycast_all(*ray, {.data = out_hits, .size = capacity}, query_filter);
		return 1;
	}

	u8 api_physics_linecast_closest(const world_t* world, const physics_linecast_t* line, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(line != nullptr);
		SFG_ASSERT(out_hit != nullptr);

		*out_hit					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		return physics.linecast_closest(*line, *out_hit, query_filter) ? 1 : 0;
	}

	void api_physics_linecast_closest_batch(const world_t* world, const physics_linecast_t* lines, physics_hit_t* out_hits, u32 count, const physics_query_filter_t* filter)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(lines != nullptr || count == 0);
		SFG_ASSERT(out_hits != nullptr || count == 0);

		if (count == 0)
			return;

		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
		{
			for (u32 i = 0; i < count; ++i)
				out_hits[i] = {};

			return;
		}

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		physics.linecast_closest_batch({.data = lines, .size = count}, {.data = out_hits, .size = count}, query_filter);
	}

	u8 api_physics_spherecast_any(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(sphere != nullptr);

		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		return physics.spherecast_any(*sphere, query_filter) ? 1 : 0;
	}

	u8 api_physics_spherecast_closest(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hit)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(sphere != nullptr);
		SFG_ASSERT(out_hit != nullptr);

		*out_hit					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		return physics.spherecast_closest(*sphere, *out_hit, query_filter) ? 1 : 0;
	}

	u8 api_physics_spherecast_all(const world_t* world, const physics_spherecast_t* sphere, const physics_query_filter_t* filter, physics_hit_t* out_hits, u32 capacity, physics_query_result_t* out_result)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(sphere != nullptr);
		SFG_ASSERT(out_hits != nullptr || capacity == 0);
		SFG_ASSERT(out_result != nullptr);

		*out_result					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init())
			return 0;

		const physics_query_filter_t query_filter = filter == nullptr ? physics_query_filter_t{} : *filter;
		*out_result								  = physics.spherecast_all(*sphere, {.data = out_hits, .size = capacity}, query_filter);
		return 1;
	}

	u8 api_physics_set_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(velocity != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_character(entity))
			return 0;

		physics.set_character_velocity(entity, *velocity);
		return 1;
	}

	u8 api_physics_add_character_velocity(world_t* world, entity_id_t entity, const vec3f_t* velocity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(velocity != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_character(entity))
			return 0;

		physics.add_character_velocity(entity, *velocity);
		return 1;
	}

	u8 api_physics_jump_character(world_t* world, entity_id_t entity, f32 speed)
	{
		SFG_ASSERT(world != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_character(entity))
			return 0;

		physics.jump_character(entity, speed);
		return 1;
	}

	u8 api_physics_teleport_character(world_t* world, entity_id_t entity, const vec3f_t* position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);

		physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_character(entity))
			return 0;

		physics.teleport_character(entity, *position);
		return 1;
	}

	u8 api_physics_get_character_state(const world_t* world, entity_id_t entity, character_mover_state_t* out_state)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_state != nullptr);

		*out_state					   = {};
		const physics_world_t& physics = world->get_physics();

		if (!physics.is_init() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity) || !physics.is_character(entity))
			return 0;

		return physics.get_character_state(entity, *out_state) ? 1 : 0;
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
