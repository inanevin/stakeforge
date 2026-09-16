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

#include "world_particle_simulation.hpp"
#include "ecs.hpp"
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
	void world_particle_simulation_t::init(world_t& world, const world_particle_simulation_config_t& config)
	{
		SFG_ASSERT(_world == nullptr);
		SFG_ASSERT(config.fixed_step_seconds > 0.0f);
		SFG_ASSERT(config.max_steps_per_tick != 0);

		_world	= &world;
		_config = config;
		_particles.init(config.page_size);
	}

	void world_particle_simulation_t::uninit()
	{
		clear();
		_particles.uninit();
		_config = {};
		_world	= nullptr;
	}

	void world_particle_simulation_t::clear()
	{
		_world->get_component_table<component_system_particle_emitter_t>().clear();
		_particles.reset();
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
		const ecs_component_table_t& system_table = _world->get_component_table<component_system_particle_emitter_t>();

		if (system_table.has(entity))
			dealloc_for_entity(entity);
	}

	void world_particle_simulation_t::tick(f32 delta_time)
	{
		ZoneScoped;

		sync_emitters();

		_fixed_accumulator += math::max(delta_time, 0.0f);
		u32 step_count = 0;

		while (_fixed_accumulator >= _config.fixed_step_seconds && step_count < _config.max_steps_per_tick)
		{
			simulate_step(_config.fixed_step_seconds);

			_fixed_accumulator -= _config.fixed_step_seconds;
			++step_count;
		}

		if (step_count == _config.max_steps_per_tick)
			_fixed_accumulator = math::min(_fixed_accumulator, _config.fixed_step_seconds);

		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		const ecs_component_table_t&	system_table = _world->get_component_table<component_system_particle_emitter_t>();
		const ecs_component_table_ref_t refs[]		 = {emitter_table.ref(), transform_table.ref(), system_table.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_particle_emitter_t&	 emitter   = row.get<component_particle_emitter_t>();
			const component_system_transform_t&	 transform = row.get<component_system_transform_t>();
			component_system_particle_emitter_t& system	   = row.get_mutable<component_system_particle_emitter_t>();

			update_bounds(system, emitter, transform);
		}
	}

	void world_particle_simulation_t::play(entity_id_t entity)
	{
		component_system_particle_emitter_t& system = _world->get_component_table<component_system_particle_emitter_t>().get_as<component_system_particle_emitter_t>(entity);

		system.playing = 1;
	}

	void world_particle_simulation_t::stop(entity_id_t entity, bool clear_particles)
	{
		component_system_particle_emitter_t& system = _world->get_component_table<component_system_particle_emitter_t>().get_as<component_system_particle_emitter_t>(entity);

		system.playing = 0;

		if (clear_particles)
			system.particle_count = 0;
	}

	void world_particle_simulation_t::restart(entity_id_t entity)
	{
		component_system_particle_emitter_t& system = _world->get_component_table<component_system_particle_emitter_t>().get_as<component_system_particle_emitter_t>(entity);

		system.particle_count		= 0;
		system.emitter_age			= 0.0f;
		system.emission_accumulator = 0.0f;
		system.spawn_serial			= 0;
		system.completed_loops		= 0;
		system.burst_emitted		= 0;
		system.playing				= 1;
	}

	const aabb_t* world_particle_simulation_t::find_bounds(entity_id_t entity) const
	{
		const component_system_particle_emitter_t* system = _world->get_component_table<component_system_particle_emitter_t>().find_as_const<component_system_particle_emitter_t>(entity);

		return system != nullptr ? &system->bounds : nullptr;
	}

	void world_particle_simulation_t::alloc_for_entity(entity_id_t entity)
	{
		const component_particle_emitter_t&	 emitter   = _world->get_component_table<component_particle_emitter_t>().get_as_const<component_particle_emitter_t>(entity);
		const component_system_transform_t&	 transform = _world->get_component_table<component_system_transform_t>().get_as_const<component_system_transform_t>(entity);
		component_system_particle_emitter_t& system	   = _world->get_component_table<component_system_particle_emitter_t>().add_or_get_as<component_system_particle_emitter_t>(entity);

		system.particles	 = _particles.allocate<particle_state_t>(emitter.max_particles);
		system.max_particles = emitter.max_particles;

		reset_emitter(system, emitter, transform);
		update_bounds(system, emitter, transform);
	}

	void world_particle_simulation_t::reset_emitter(component_system_particle_emitter_t& system, const component_particle_emitter_t& emitter, const component_system_transform_t& transform)
	{
		system.particle_count		= 0;
		system.bounds				= {};
		system.emitter_age			= 0.0f;
		system.emission_accumulator = 0.0f;
		system.spawn_serial			= 0;
		system.completed_loops		= 0;
		system.burst_emitted		= 0;
		system.playing				= emitter.play_on_create;

		if (emitter.prewarm != 0 && emitter.loop_mode != particle_loop_mode_e::once && system.playing != 0)
		{
			const u32 prewarm_steps = math::min(static_cast<u32>(math::ceil(emitter.duration / _config.fixed_step_seconds)), _config.prewarm_max_steps);

			for (u32 step = 0; step < prewarm_steps; ++step)
				simulate_emitter(system, emitter, transform, _config.fixed_step_seconds);

			const f32 active_age = system.emitter_age - emitter.start_delay;

			if (active_age >= emitter.duration)
			{
				system.emitter_age	 = emitter.start_delay + math::fmodf(active_age, emitter.duration);
				system.burst_emitted = 0;
			}

			system.completed_loops = 0;
			system.playing		   = emitter.play_on_create;
		}
	}

	void world_particle_simulation_t::reset_emitters()
	{
		sync_emitters();

		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		_fixed_accumulator = 0.0f;

		const ecs_component_table_t&	system_table = _world->get_component_table<component_system_particle_emitter_t>();
		const ecs_component_table_ref_t refs[]		 = {emitter_table.ref(), transform_table.ref(), system_table.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_particle_emitter_t&	 emitter   = row.get<component_particle_emitter_t>();
			const component_system_transform_t&	 transform = row.get<component_system_transform_t>();
			component_system_particle_emitter_t& system	   = row.get_mutable<component_system_particle_emitter_t>();

			reset_emitter(system, emitter, transform);
			update_bounds(system, emitter, transform);
		}
	}

	void world_particle_simulation_t::dealloc_for_entity(entity_id_t entity)
	{
		ecs_component_table_t&				 system_table = _world->get_component_table<component_system_particle_emitter_t>();
		component_system_particle_emitter_t& system		  = system_table.get_as<component_system_particle_emitter_t>(entity);

		_particles.free(system.particles);
		system_table.remove(entity);
	}

	void world_particle_simulation_t::sync_emitters()
	{
		ZoneScoped;

		const ecs_component_table_t& emitter_table		 = _world->get_component_table<component_particle_emitter_t>();
		const ecs_component_table_t& disabled_table		 = _world->get_component_table<component_disabled_t>();
		const ecs_component_table_t& system_table		 = _world->get_component_table<component_system_particle_emitter_t>();
		frame_vector_t<entity_id_t>	 add_sys_entities	 = {};
		frame_vector_t<entity_id_t>	 remove_sys_entities = {};

		// engine emitter no system emitter, create
		{
			const ecs_component_table_ref_t refs[] = {emitter_table.ref(), !system_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				add_sys_entities.push_back(row.id);
		}

		// system emitter disabled, remove
		{
			const ecs_component_table_ref_t refs[] = {system_table.ref(), disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				remove_sys_entities.push_back(row.id);
		}

		// system emitter not disabled but no engine emitter, remove
		{
			const ecs_component_table_ref_t refs[] = {system_table.ref(), !emitter_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				remove_sys_entities.push_back(row.id);
		}

		{
			const ecs_component_table_ref_t refs[] = {emitter_table.ref(), system_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_particle_emitter_t&		   emitter = row.get<component_particle_emitter_t>();
				const component_system_particle_emitter_t& system  = row.get<component_system_particle_emitter_t>();

				if (emitter.max_particles == system.max_particles)
					continue;

				remove_sys_entities.push_back(row.id);
				add_sys_entities.push_back(row.id);
			}
		}

		for (const entity_id_t entity : remove_sys_entities)
			dealloc_for_entity(entity);

		for (const entity_id_t entity : add_sys_entities)
			alloc_for_entity(entity);
	}

	void world_particle_simulation_t::simulate_step(f32 delta_time)
	{
		ZoneScoped;

		const ecs_component_table_t& emitter_table	 = _world->get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		const ecs_component_table_t&	system_table = _world->get_component_table<component_system_particle_emitter_t>();
		const ecs_component_table_ref_t refs[]		 = {emitter_table.ref(), transform_table.ref(), system_table.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_particle_emitter_t&	 emitter   = row.get<component_particle_emitter_t>();
			const component_system_transform_t&	 transform = row.get<component_system_transform_t>();
			component_system_particle_emitter_t& system	   = row.get_mutable<component_system_particle_emitter_t>();

			simulate_emitter(system, emitter, transform, delta_time);
		}
	}

	void world_particle_simulation_t::simulate_emitter(component_system_particle_emitter_t& system, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, f32 delta_time)
	{
		const curve_runtime_t* acceleration_curve	= emitter.acceleration_over_lifetime != NULL_RESOURCE_HANDLE ? resource_manager_t::get().find_runtime<curve_runtime_t>(emitter.acceleration_over_lifetime) : nullptr;
		const vec3f_t		   gravity				= _config.gravity * emitter.gravity_multiplier;
		const vec3f_t		   gravity_acceleration = emitter.simulation_space == particle_simulation_space_e::world ? gravity : transform.abs_rot.conjugate() * gravity;
		const f32			   damping				= math::max(0.0f, 1.0f - emitter.drag * delta_time);

		particle_state_t* const particles = _particles.get<particle_state_t>(system.particles);

		for (u32 particle_index = 0; particle_index < system.particle_count;)
		{
			particle_state_t& particle = particles[particle_index];

			particle.age += delta_time;

			if (particle.age >= particle.lifetime)
			{
				particle = particles[--system.particle_count];
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

		if (system.playing == 0)
			return;

		system.emitter_age += delta_time;

		if (system.emitter_age < emitter.start_delay)
			return;

		const f32 active_age = system.emitter_age - emitter.start_delay;

		if (active_age >= emitter.duration)
		{
			const u32 elapsed_loops = static_cast<u32>(math::floor(active_age / emitter.duration));

			if (emitter.loop_mode == particle_loop_mode_e::once)
			{
				system.playing = 0;
				return;
			}

			if (emitter.loop_mode == particle_loop_mode_e::loop_count)
			{
				if (system.completed_loops + elapsed_loops >= emitter.loop_count)
				{
					system.playing = 0;
					return;
				}

				system.completed_loops += elapsed_loops;
			}

			system.emitter_age	 = emitter.start_delay + math::fmodf(active_age, emitter.duration);
			system.burst_emitted = 0;
		}

		u32 emit_count = 0;

		if (system.burst_emitted == 0)
		{
			emit_count += emitter.burst_count;
			system.burst_emitted = 1;
		}

		system.emission_accumulator += emitter.emission_rate * delta_time;

		const u32 continuous_count = static_cast<u32>(math::floor(system.emission_accumulator));

		system.emission_accumulator -= static_cast<f32>(continuous_count);
		emit_count += continuous_count;

		if (emit_count != 0)
			emit_particles(system, emitter, transform, emit_count);
	}

	void world_particle_simulation_t::emit_particles(component_system_particle_emitter_t& system, const component_particle_emitter_t& emitter, const component_system_transform_t& transform, u32 count)
	{
		const u32				emitter_available = system.max_particles - system.particle_count;
		const u32				spawn_count		  = math::min(count, emitter_available);
		particle_state_t* const particles		  = _particles.get<particle_state_t>(system.particles);

		for (u32 particle_index = 0; particle_index < spawn_count; ++particle_index)
		{
			u32 random_state = emitter.random_seed ^ (system.spawn_serial++ * 747796405u + 2891336453u);

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

			particles[system.particle_count++] = particle;
		}
	}

	void world_particle_simulation_t::update_bounds(component_system_particle_emitter_t& system, const component_particle_emitter_t& emitter, const component_system_transform_t& transform)
	{
		ZoneScoped;

		if (system.particle_count == 0)
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

				local_half_extent = {cone_radius, cone_radius, emitter.cone_length * 0.5f};
				local_center	  = {0.0f, 0.0f, -emitter.cone_length * 0.5f};
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

						bounds_min = vec3f_t::min(bounds_min, corner);
						bounds_max = vec3f_t::max(bounds_max, corner);
					}
				}
			}

			system.bounds = {bounds_min, bounds_max};
			return;
		}

		const particle_state_t* particles			  = _particles.get<particle_state_t>(system.particles);
		const particle_state_t& first				  = particles[0];
		const curve_runtime_t*	size_curve			  = emitter.size_over_lifetime != NULL_RESOURCE_HANDLE ? resource_manager_t::get().find_runtime<curve_runtime_t>(emitter.size_over_lifetime) : nullptr;
		const vec3f_t			first_position		  = emitter.simulation_space == particle_simulation_space_e::world ? first.position : transform.abs_mat * first.position;
		const f32				first_normalized_age  = first.age / first.lifetime;
		const f32				local_scale			  = emitter.simulation_space == particle_simulation_space_e::local ? math::max(math::abs(transform.abs_scale.x), math::max(math::abs(transform.abs_scale.y), math::abs(transform.abs_scale.z))) : 1.0f;
		const f32				first_size_multiplier = (size_curve != nullptr ? size_curve->sample(first_normalized_age).x : 1.0f) * emitter.size_amplitude;
		const f32				first_radius		  = first.start_size * math::max(first_size_multiplier, 0.0f) * local_scale * 0.5f;
		vec3f_t					bounds_min			  = first_position - vec3f_t{first_radius, first_radius, first_radius};
		vec3f_t					bounds_max			  = first_position + vec3f_t{first_radius, first_radius, first_radius};

		for (u32 particle_index = 1; particle_index < system.particle_count; ++particle_index)
		{
			const particle_state_t& particle		= particles[particle_index];
			const vec3f_t			position		= emitter.simulation_space == particle_simulation_space_e::world ? particle.position : transform.abs_mat * particle.position;
			const f32				normalized_age	= particle.age / particle.lifetime;
			const f32				size_multiplier = (size_curve != nullptr ? size_curve->sample(normalized_age).x : 1.0f) * emitter.size_amplitude;
			const f32				radius			= particle.start_size * math::max(size_multiplier, 0.0f) * local_scale * 0.5f;
			const vec3f_t			extent			= {radius, radius, radius};

			bounds_min = vec3f_t::min(bounds_min, position - extent);
			bounds_max = vec3f_t::max(bounds_max, position + extent);
		}

		system.bounds = {bounds_min, bounds_max};
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
