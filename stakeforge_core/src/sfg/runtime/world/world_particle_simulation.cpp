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

#include "world_particle_simulation.hpp"
#include "ecs.hpp"
#include "ecs_helpers.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/color_utils.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/random.hpp>
#include <sfg/runtime/resources/curve.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

#include <tracy/Tracy.hpp>

namespace sfg
{
#define PARTICLE_SIMULATION_FIXED_STEP		  (1.0f / 60.0f)
#define PARTICLE_SIMULATION_MAX_STEPS		  4
#define PARTICLE_SIMULATION_EMITTER_RESERVE	  32
#define PARTICLE_SIMULATION_PREWARM_MAX_STEPS 240

	void world_particle_simulation_t::init(world_t& world)
	{
		SFG_ASSERT(_world == nullptr);

		_world = &world;
		_emitters.reserve(PARTICLE_SIMULATION_EMITTER_RESERVE);
	}

	void world_particle_simulation_t::uninit()
	{
		clear();
		_emitters.shrink_to_fit();
		_world = nullptr;
	}

	void world_particle_simulation_t::clear()
	{
		_emitters.resize(0);
		_fixed_accumulator = 0.0f;
	}

	void world_particle_simulation_t::begin_play()
	{
		reset_emitters();
	}

	void world_particle_simulation_t::end_play()
	{
		reset_emitters();
	}

	void world_particle_simulation_t::destroy_entity(entity_id_t entity)
	{
		const ecs_component_table_t&			   system_table = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const component_system_particle_emitter_t* system		= ecs_helpers_t::table_find_as_const<component_system_particle_emitter_t>(system_table, entity);

		if (system != nullptr && system->runtime_index < _emitters.size() && _emitters[system->runtime_index].entity == entity)
			remove_runtime(system->runtime_index);
	}

	void world_particle_simulation_t::tick(f32 delta_time)
	{
		ZoneScoped;

		sync_emitters();

		_fixed_accumulator += math::max(delta_time, 0.0f);
		u32 step_count = 0;

		while (_fixed_accumulator >= PARTICLE_SIMULATION_FIXED_STEP && step_count < PARTICLE_SIMULATION_MAX_STEPS)
		{
			simulate_step(PARTICLE_SIMULATION_FIXED_STEP);
			_fixed_accumulator -= PARTICLE_SIMULATION_FIXED_STEP;
			++step_count;
		}

		if (step_count == PARTICLE_SIMULATION_MAX_STEPS)
			_fixed_accumulator = math::min(_fixed_accumulator, PARTICLE_SIMULATION_FIXED_STEP);

		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		for (particle_emitter_runtime_t& runtime : _emitters)
		{
			const component_particle_emitter_t& emitter	  = ecs_helpers_t::table_get_as_const<component_particle_emitter_t>(emitter_table, runtime.entity);
			const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, runtime.entity);

			update_bounds(runtime, emitter, transform);
		}
	}

	void world_particle_simulation_t::play(entity_id_t entity)
	{
		const ecs_component_table_t&			   system_table = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const component_system_particle_emitter_t& system		= ecs_helpers_t::table_get_as_const<component_system_particle_emitter_t>(system_table, entity);
		particle_emitter_runtime_t&				   runtime		= _emitters[system.runtime_index];

		runtime.playing = 1;
	}

	void world_particle_simulation_t::stop(entity_id_t entity, bool clear_particles)
	{
		const ecs_component_table_t&			   system_table = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const component_system_particle_emitter_t& system		= ecs_helpers_t::table_get_as_const<component_system_particle_emitter_t>(system_table, entity);
		particle_emitter_runtime_t&				   runtime		= _emitters[system.runtime_index];

		runtime.playing = 0;

		if (clear_particles)
			runtime.particles.resize(0);
	}

	void world_particle_simulation_t::restart(entity_id_t entity)
	{
		const ecs_component_table_t&			   system_table = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const component_system_particle_emitter_t& system		= ecs_helpers_t::table_get_as_const<component_system_particle_emitter_t>(system_table, entity);
		particle_emitter_runtime_t&				   runtime		= _emitters[system.runtime_index];

		runtime.particles.resize(0);
		runtime.emitter_age			 = 0.0f;
		runtime.emission_accumulator = 0.0f;
		runtime.spawn_serial		 = 0;
		runtime.completed_loops		 = 0;
		runtime.burst_emitted		 = 0;
		runtime.playing				 = 1;
	}

	const particle_emitter_runtime_t* world_particle_simulation_t::find_runtime(entity_id_t entity) const
	{
		const ecs_component_table_t&			   system_table = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const component_system_particle_emitter_t* system		= ecs_helpers_t::table_find_as_const<component_system_particle_emitter_t>(system_table, entity);

		if (system == nullptr || system->runtime_index >= _emitters.size() || _emitters[system->runtime_index].entity != entity)
			return nullptr;

		return &_emitters[system->runtime_index];
	}

	const aabb_t* world_particle_simulation_t::find_bounds(entity_id_t entity) const
	{
		const particle_emitter_runtime_t* runtime = find_runtime(entity);
		return runtime != nullptr ? &runtime->bounds : nullptr;
	}

	particle_emitter_runtime_t& world_particle_simulation_t::create_runtime(entity_id_t entity, const component_particle_emitter_t& emitter, const component_system_transform_t& transform)
	{
		particle_emitter_runtime_t& runtime = _emitters.emplace_back();
		runtime.entity						= entity;

		reset_runtime(runtime, emitter, transform);

		return runtime;
	}

	void world_particle_simulation_t::reset_runtime(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform)
	{
		runtime.particles.resize(0);
		runtime.bounds				 = {};
		runtime.emitter_age			 = 0.0f;
		runtime.emission_accumulator = 0.0f;
		runtime.spawn_serial		 = 0;
		runtime.completed_loops		 = 0;
		runtime.burst_emitted		 = 0;
		runtime.playing				 = emitter.play_on_create;
		runtime.particles.reserve(emitter.max_particles);

		if (emitter.prewarm != 0 && emitter.loop_mode != particle_loop_mode_e::once && runtime.playing != 0)
		{
			const u32 prewarm_steps = math::min(static_cast<u32>(math::ceil(emitter.duration / PARTICLE_SIMULATION_FIXED_STEP)), static_cast<u32>(PARTICLE_SIMULATION_PREWARM_MAX_STEPS));

			for (u32 step = 0; step < prewarm_steps; ++step)
				simulate_emitter(runtime, emitter, transform, PARTICLE_SIMULATION_FIXED_STEP);

			const f32 active_age = runtime.emitter_age - emitter.start_delay;

			if (active_age >= emitter.duration)
			{
				runtime.emitter_age	  = emitter.start_delay + math::fmodf(active_age, emitter.duration);
				runtime.burst_emitted = 0;
			}

			runtime.completed_loops = 0;
			runtime.playing			= emitter.play_on_create;
		}
	}

	void world_particle_simulation_t::reset_emitters()
	{
		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		_fixed_accumulator = 0.0f;

		for (particle_emitter_runtime_t& runtime : _emitters)
		{
			const component_particle_emitter_t& emitter	  = ecs_helpers_t::table_get_as_const<component_particle_emitter_t>(emitter_table, runtime.entity);
			const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, runtime.entity);

			reset_runtime(runtime, emitter, transform);
			update_bounds(runtime, emitter, transform);
		}
	}

	void world_particle_simulation_t::remove_runtime(u32 runtime_index)
	{
		ecs_component_table_t& system_table	  = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const entity_id_t	   removed_entity = _emitters[runtime_index].entity;
		const u32			   last_index	  = static_cast<u32>(_emitters.size() - 1);

		if (runtime_index != last_index)
		{
			_emitters[runtime_index]						  = std::move(_emitters[last_index]);
			component_system_particle_emitter_t& moved_system = ecs_helpers_t::table_get_as<component_system_particle_emitter_t>(system_table, _emitters[runtime_index].entity);
			moved_system.runtime_index						  = runtime_index;
		}

		_emitters.pop_back();
		ecs_t::table_remove(system_table, removed_entity);
	}

	void world_particle_simulation_t::sync_emitters()
	{
		ZoneScoped;

		const ecs_component_table_t& alive_table	 = _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& disabled_table	 = _world->get_component_table(type_id_t<component_disabled_t>::value);
		ecs_component_table_t&		 system_table	 = _world->get_component_table(type_id_t<component_system_particle_emitter_t>::value);

		// engine emitter no system emitter, create
		{
			const ecs_component_table_ref_t refs[] = {
				!disabled_table.ref(),
				alive_table.ref(),
				emitter_table.ref(),
				!system_table.ref(),
			};
			frame_vector_t<entity_id_t> create_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				create_entities.push_back(row.id);

			for (const entity_id_t entity : create_entities)
			{
				const component_particle_emitter_t& emitter	  = ecs_helpers_t::table_get_as_const<component_particle_emitter_t>(emitter_table, entity);
				const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);

				create_runtime(entity, emitter, transform);

				component_system_particle_emitter_t& system = ecs_helpers_t::table_add_or_get_as<component_system_particle_emitter_t>(system_table, entity);
				system.runtime_index						= static_cast<u32>(_emitters.size() - 1);
			}
		}

		// system emitter disabled, remove
		{
			const ecs_component_table_ref_t refs[] = {
				alive_table.ref(),
				disabled_table.ref(),
				system_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				destroy_entities.push_back(row.id);

			for (const entity_id_t entity : destroy_entities)
			{
				const component_system_particle_emitter_t& system = ecs_helpers_t::table_get_as_const<component_system_particle_emitter_t>(system_table, entity);
				remove_runtime(system.runtime_index);
			}
		}

		// system emitter not disabled but no engine emitter, remove
		{
			const ecs_component_table_ref_t refs[] = {
				alive_table.ref(),
				system_table.ref(),
				!emitter_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				destroy_entities.push_back(row.id);

			for (const entity_id_t entity : destroy_entities)
			{
				const component_system_particle_emitter_t& system = ecs_helpers_t::table_get_as_const<component_system_particle_emitter_t>(system_table, entity);
				remove_runtime(system.runtime_index);
			}
		}
	}

	void world_particle_simulation_t::simulate_step(f32 delta_time)
	{
		ZoneScoped;

		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		for (particle_emitter_runtime_t& runtime : _emitters)
		{
			const component_particle_emitter_t& emitter	  = ecs_helpers_t::table_get_as_const<component_particle_emitter_t>(emitter_table, runtime.entity);
			const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, runtime.entity);

			simulate_emitter(runtime, emitter, transform, delta_time);
		}
	}

	void world_particle_simulation_t::simulate_emitter(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, f32 delta_time)
	{
		const curve_runtime_t* acceleration_curve	= emitter.acceleration_over_lifetime != NULL_RESOURCE_HANDLE ? resource_manager_t::get().find_runtime<curve_runtime_t>(emitter.acceleration_over_lifetime) : nullptr;
		const vec3f_t		   gravity				= {0.0f, -9.81f * emitter.gravity_multiplier, 0.0f};
		const vec3f_t		   gravity_acceleration = emitter.simulation_space == particle_simulation_space_e::world ? gravity : transform.abs_rot.conjugate() * gravity;
		const f32			   damping				= math::max(0.0f, 1.0f - emitter.drag * delta_time);

		for (size_t particle_index = 0; particle_index < runtime.particles.size();)
		{
			particle_state_t& particle = runtime.particles[particle_index];
			particle.age += delta_time;

			if (particle.age >= particle.lifetime)
			{
				particle = runtime.particles.back();
				runtime.particles.pop_back();
				continue;
			}

			particle.previous_position = particle.position;

			const f32	  normalized_age	  = particle.age / particle.lifetime;
			const vec4f_t acceleration_sample = acceleration_curve != nullptr ? acceleration_curve->sample(normalized_age) : vec4f_t::zero;
			const vec3f_t acceleration		  = gravity_acceleration + vec3f_t{
																		   acceleration_sample.x * emitter.acceleration_amplitude.x,
																		   acceleration_sample.y * emitter.acceleration_amplitude.y,
																		   acceleration_sample.z * emitter.acceleration_amplitude.z,
																	   };

			particle.velocity += acceleration * delta_time;
			particle.velocity = particle.velocity * damping;
			particle.position += particle.velocity * delta_time;
			particle.rotation += particle.angular_velocity * delta_time;
			++particle_index;
		}

		if (runtime.playing == 0)
			return;

		runtime.emitter_age += delta_time;

		if (runtime.emitter_age < emitter.start_delay)
			return;

		const f32 active_age = runtime.emitter_age - emitter.start_delay;

		if (active_age >= emitter.duration)
		{
			const u32 elapsed_loops = static_cast<u32>(math::floor(active_age / emitter.duration));

			if (emitter.loop_mode == particle_loop_mode_e::once)
			{
				runtime.playing = 0;
				return;
			}

			if (emitter.loop_mode == particle_loop_mode_e::loop_count)
			{
				if (runtime.completed_loops + elapsed_loops >= emitter.loop_count)
				{
					runtime.playing = 0;
					return;
				}

				runtime.completed_loops += elapsed_loops;
			}

			runtime.emitter_age	  = emitter.start_delay + math::fmodf(active_age, emitter.duration);
			runtime.burst_emitted = 0;
		}

		u32 emit_count = 0;

		if (runtime.burst_emitted == 0)
		{
			emit_count += emitter.burst_count;
			runtime.burst_emitted = 1;
		}

		runtime.emission_accumulator += emitter.emission_rate * delta_time;
		const u32 continuous_count = static_cast<u32>(math::floor(runtime.emission_accumulator));
		runtime.emission_accumulator -= static_cast<f32>(continuous_count);
		emit_count += continuous_count;

		if (emit_count != 0)
			emit_particles(runtime, emitter, transform, emit_count);
	}

	void world_particle_simulation_t::emit_particles(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, u32 count)
	{
		const u32 available	  = emitter.max_particles - static_cast<u32>(runtime.particles.size());
		const u32 spawn_count = math::min(count, available);

		for (u32 particle_index = 0; particle_index < spawn_count; ++particle_index)
		{
			u32 random_state = emitter.random_seed ^ (runtime.spawn_serial++ * 747796405u + 2891336453u);

			if (random_state == 0)
				random_state = 1;

			const vec3f_t local_position = random_spawn_position(random_state, emitter);
			const vec3f_t local_velocity = {
				random_t::random_range(random_state, emitter.velocity_min.x, emitter.velocity_max.x),
				random_t::random_range(random_state, emitter.velocity_min.y, emitter.velocity_max.y),
				random_t::random_range(random_state, emitter.velocity_min.z, emitter.velocity_max.z),
			};

			const f32 color_alpha = random_t::random_01(random_state);

			const particle_state_t particle{
				.start_color	   = color_utils_t::lerp(emitter.start_color_min, emitter.start_color_max, color_alpha),
				.position		   = emitter.simulation_space == particle_simulation_space_e::world ? transform.abs_mat * local_position : local_position,
				.previous_position = emitter.simulation_space == particle_simulation_space_e::world ? transform.abs_mat * local_position : local_position,
				.velocity		   = emitter.simulation_space == particle_simulation_space_e::world ? transform.abs_rot * local_velocity : local_velocity,
				.age			   = 0.0f,
				.lifetime		   = math::max(random_t::random_range(random_state, emitter.lifetime_min, emitter.lifetime_max), 0.001f),
				.start_size		   = math::max(random_t::random_range(random_state, emitter.start_size_min, emitter.start_size_max), 0.0f),
				.rotation		   = random_t::random_range(random_state, emitter.start_rotation_min, emitter.start_rotation_max),
				.angular_velocity  = random_t::random_range(random_state, emitter.angular_velocity_min, emitter.angular_velocity_max),
				.random			   = random_state,
			};

			runtime.particles.push_back(particle);
		}
	}

	void world_particle_simulation_t::update_bounds(particle_emitter_runtime_t& runtime, const component_particle_emitter_t& emitter, const component_system_transform_t& transform)
	{
		ZoneScoped;

		if (runtime.particles.empty())
		{
			vec3f_t local_half_extent = vec3f_t::zero;
			vec3f_t local_center	  = vec3f_t::zero;

			switch (emitter.shape)
			{
			case particle_spawn_shape_e::point:
				break;
			case particle_spawn_shape_e::box:
				local_half_extent = vec3f_t::abs(emitter.box_half_extents);
				break;
			case particle_spawn_shape_e::sphere:
				local_half_extent = {emitter.shape_radius, emitter.shape_radius, emitter.shape_radius};
				break;
			case particle_spawn_shape_e::cone: {
				const f32 cone_radius = math::tan(math::degrees_to_radians(emitter.cone_angle_degrees)) * emitter.cone_length;
				local_half_extent	  = {cone_radius, cone_radius, emitter.cone_length * 0.5f};
				local_center		  = {0.0f, 0.0f, -emitter.cone_length * 0.5f};
				break;
			}
			}

			const vec3f_t first_corner = transform.abs_mat * (local_center - local_half_extent);
			vec3f_t		  bounds_min   = first_corner;
			vec3f_t		  bounds_max   = first_corner;

			for (u32 x = 0; x < 2; ++x)
			{
				for (u32 y = 0; y < 2; ++y)
				{
					for (u32 z = 0; z < 2; ++z)
					{
						const vec3f_t corner = transform.abs_mat * vec3f_t{
																	   local_center.x + (x == 0 ? -local_half_extent.x : local_half_extent.x),
																	   local_center.y + (y == 0 ? -local_half_extent.y : local_half_extent.y),
																	   local_center.z + (z == 0 ? -local_half_extent.z : local_half_extent.z),
																   };
						bounds_min			 = vec3f_t::min(bounds_min, corner);
						bounds_max			 = vec3f_t::max(bounds_max, corner);
					}
				}
			}

			runtime.bounds = {bounds_min, bounds_max};
			return;
		}

		const particle_state_t& first				  = runtime.particles.front();
		const curve_runtime_t*	size_curve			  = emitter.size_over_lifetime != NULL_RESOURCE_HANDLE ? resource_manager_t::get().find_runtime<curve_runtime_t>(emitter.size_over_lifetime) : nullptr;
		const vec3f_t			first_position		  = emitter.simulation_space == particle_simulation_space_e::world ? first.position : transform.abs_mat * first.position;
		const f32				first_normalized_age  = first.age / first.lifetime;
		const f32				local_scale			  = emitter.simulation_space == particle_simulation_space_e::local ? math::max(math::abs(transform.abs_scale.x), math::max(math::abs(transform.abs_scale.y), math::abs(transform.abs_scale.z))) : 1.0f;
		const f32				first_size_multiplier = (size_curve != nullptr ? size_curve->sample(first_normalized_age).x : 1.0f) * emitter.size_amplitude;
		const f32				first_radius		  = first.start_size * math::max(first_size_multiplier, 0.0f) * local_scale * 0.5f;
		vec3f_t					bounds_min			  = first_position - vec3f_t{first_radius, first_radius, first_radius};
		vec3f_t					bounds_max			  = first_position + vec3f_t{first_radius, first_radius, first_radius};

		for (size_t particle_index = 1; particle_index < runtime.particles.size(); ++particle_index)
		{
			const particle_state_t& particle		= runtime.particles[particle_index];
			const vec3f_t			position		= emitter.simulation_space == particle_simulation_space_e::world ? particle.position : transform.abs_mat * particle.position;
			const f32				normalized_age	= particle.age / particle.lifetime;
			const f32				size_multiplier = (size_curve != nullptr ? size_curve->sample(normalized_age).x : 1.0f) * emitter.size_amplitude;
			const f32				radius			= particle.start_size * math::max(size_multiplier, 0.0f) * local_scale * 0.5f;
			const vec3f_t			extent			= {radius, radius, radius};
			bounds_min								= vec3f_t::min(bounds_min, position - extent);
			bounds_max								= vec3f_t::max(bounds_max, position + extent);
		}

		runtime.bounds = {bounds_min, bounds_max};
	}

	vec3f_t world_particle_simulation_t::random_spawn_position(u32& random_state, const component_particle_emitter_t& emitter) const
	{
		switch (emitter.shape)
		{
		case particle_spawn_shape_e::point:
			return vec3f_t::zero;
		case particle_spawn_shape_e::box:
			return {
				random_t::random_range(random_state, -emitter.box_half_extents.x, emitter.box_half_extents.x),
				random_t::random_range(random_state, -emitter.box_half_extents.y, emitter.box_half_extents.y),
				random_t::random_range(random_state, -emitter.box_half_extents.z, emitter.box_half_extents.z),
			};
		case particle_spawn_shape_e::sphere: {
			vec3f_t point = vec3f_t::zero;

			for (u32 attempt = 0; attempt < 8; ++attempt)
			{
				point = {
					random_t::random_range(random_state, -1.0f, 1.0f),
					random_t::random_range(random_state, -1.0f, 1.0f),
					random_t::random_range(random_state, -1.0f, 1.0f),
				};

				if (point.magnitude_sqr() <= 1.0f)
					return point * emitter.shape_radius;
			}

			return point.normalized() * emitter.shape_radius;
		}

		case particle_spawn_shape_e::cone: {
			const f32 distance = emitter.cone_length * random_t::random_01(random_state);
			const f32 radius   = math::tan(math::degrees_to_radians(emitter.cone_angle_degrees)) * distance * math::sqrt(random_t::random_01(random_state));
			const f32 angle	   = random_t::random_01(random_state) * MATH_PI * 2.0f;
			return {math::cos(angle) * radius, math::sin(angle) * radius, -distance};
		}
		}

		return vec3f_t::zero;
	}
}
