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

#include <sfg/data/span.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/runtime/physics/physics_config.hpp>
#include <sfg/runtime/physics/physics_types.hpp>

namespace sfg
{
	class world_t;
	class world_debug_draw_t;

	struct character_mover_state_t
	{
		vec3f_t		velocity			= vec3f_t::zero;
		vec3f_t		ground_normal		= vec3f_t::up;
		vec3f_t		ground_velocity		= vec3f_t::zero;
		entity_id_t ground_entity		= NULL_ENTITY_ID;
		u32			ground_sub_shape_id = 0;
		bool		is_grounded			= false;
	};

	struct physics_body_state_t
	{
		vec3f_t position		 = vec3f_t::zero;
		quat_t	rotation		 = quat_t::identity;
		vec3f_t linear_velocity	 = vec3f_t::zero;
		vec3f_t angular_velocity = vec3f_t::zero;
		bool	is_active		 = false;
	};

	class physics_world_t final
	{
	public:
		physics_world_t();
		~physics_world_t();
		physics_world_t(const physics_world_t&)			   = delete;
		physics_world_t& operator=(const physics_world_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world, const physics_runtime_config_t& config);
		void uninit();
		void clear();
		void tick(f32 delta_time);
		void draw_debug(world_debug_draw_t& debug_draw);

		// -----------------------------------------------------------------------------
		// bodies
		// -----------------------------------------------------------------------------

		void sync_body_create_destroy();
		void destroy_body(entity_id_t entity);
		void set_body_linear_velocity(entity_id_t entity, const vec3f_t& velocity);
		void set_body_angular_velocity(entity_id_t entity, const vec3f_t& velocity);
		void add_body_force(entity_id_t entity, const vec3f_t& force);
		void add_body_impulse(entity_id_t entity, const vec3f_t& impulse);
		void wake_body(entity_id_t entity);
		bool get_body_state(entity_id_t entity, physics_body_state_t& out_state) const;

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool				   raycast_any(const physics_raycast_t& ray, const physics_query_filter_t& filter = {}) const;
		bool				   raycast_closest(const physics_raycast_t& ray, physics_hit_t& out_hit, const physics_query_filter_t& filter = {}) const;
		physics_query_result_t raycast_all(const physics_raycast_t& ray, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter = {}) const;
		bool				   linecast_closest(const physics_linecast_t& line, physics_hit_t& out_hit, const physics_query_filter_t& filter = {}) const;
		void				   linecast_closest_batch(span_t<const physics_linecast_t> lines, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter = {}) const;
		bool				   spherecast_any(const physics_spherecast_t& sphere, const physics_query_filter_t& filter = {}) const;
		bool				   spherecast_closest(const physics_spherecast_t& sphere, physics_hit_t& out_hit, const physics_query_filter_t& filter = {}) const;
		physics_query_result_t spherecast_all(const physics_spherecast_t& sphere, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter = {}) const;

		// -----------------------------------------------------------------------------
		// character
		// -----------------------------------------------------------------------------

		void set_character_velocity(entity_id_t entity, const vec3f_t& velocity);
		void add_character_velocity(entity_id_t entity, const vec3f_t& velocity);
		void jump_character(entity_id_t entity, f32 speed);
		void teleport_character(entity_id_t entity, const vec3f_t& position);
		bool get_character_state(entity_id_t entity, character_mover_state_t& out_state) const;

		// -----------------------------------------------------------------------------
		// settings
		// -----------------------------------------------------------------------------

		void update_collision_masks(const u64 masks[PHYSICS_COLLISION_LAYER_MAX], u64 active_layers);
		void update_step_settings(u32 physics_rate, u32 max_sub_steps);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		span_t<const physics_contact_event_t> get_contact_events() const;

		inline bool is_init() const
		{
			return _impl != nullptr;
		}

	private:
		class impl_t;
		impl_t* _impl = nullptr;
	};
}
