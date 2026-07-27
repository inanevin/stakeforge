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

#include "engine_components.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/world_init_config.hpp>

namespace sfg
{
	class world_t;
	struct component_system_transform_t;

	struct particle_state_t
	{
		color_t start_color		  = color_t::white;
		vec3f_t position		  = vec3f_t::zero;
		vec3f_t previous_position = vec3f_t::zero;
		vec3f_t velocity		  = vec3f_t::zero;
		f32		age				  = 0.0f;
		f32		lifetime		  = 1.0f;
		f32		start_size		  = 1.0f;
		f32		rotation		  = 0.0f;
		f32		angular_velocity  = 0.0f;
		u32		random			  = 0;
	};

	struct particle_emitter_runtime_t
	{
		vector_t<particle_state_t> particles;
		aabb_t					   bounds				= {};
		entity_id_t				   entity				= NULL_ENTITY_ID;
		f32						   emitter_age			= 0.0f;
		f32						   emission_accumulator = 0.0f;
		u32						   spawn_serial			= 0;
		u32						   completed_loops		= 0;
		u8						   burst_emitted		= 0;
		u8						   playing				= 0;
	};

	class world_particle_simulation_t final
	{
	public:
		world_particle_simulation_t()											   = default;
		~world_particle_simulation_t()											   = default;
		world_particle_simulation_t(const world_particle_simulation_t&)			   = delete;
		world_particle_simulation_t& operator=(const world_particle_simulation_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world, const world_particle_simulation_config_t& config);
		void uninit();
		void clear();
		void begin_play();
		void end_play();
		void destroy_entity(entity_id_t entity);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick(f32 delta_time);
		void play(entity_id_t entity);
		void stop(entity_id_t entity, bool clear_particles);
		void restart(entity_id_t entity);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		const particle_emitter_runtime_t* find_runtime(entity_id_t entity) const;
		const aabb_t*					  find_bounds(entity_id_t entity) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline span_t<const particle_emitter_runtime_t> get_emitters() const
		{
			return {.data = _emitters.data(), .size = _emitters.size()};
		}

	private:
		particle_emitter_runtime_t& create_runtime(entity_id_t entity, const component_particle_emitter_t& emitter, const component_system_transform_t& transform);
		void						reset_runtime(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform);
		void						reset_emitters();
		void						remove_runtime(u32 runtime_index);
		void						sync_emitters();
		void						simulate_step(f32 delta_time);
		void						simulate_emitter(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, f32 delta_time);
		void						emit_particles(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, u32 count);
		void						update_bounds(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform);
		vec3f_t						random_spawn_position(u32& random_state, const component_particle_emitter_t& emitter) const;

	private:
		vector_t<particle_emitter_runtime_t> _emitters;
		world_particle_simulation_config_t	 _config			= {};
		world_t*							 _world				= nullptr;
		f32									 _fixed_accumulator = 0.0f;
		u32									 _particle_count	= 0;
	};
}
