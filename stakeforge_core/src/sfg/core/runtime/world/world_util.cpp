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

#include "world_util.hpp"
#include "ecs.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include "core/data/frame_vector.hpp"
#include "core/math/random.hpp"
#include "core/runtime/resources/mesh.hpp"
#include "core/runtime/resources/resource_manager.hpp"
#include "core/runtime/resources/sprite.hpp"

#include <tracy/Tracy.hpp>

namespace sfg
{
	void world_util_t::sync_destroyers(world_t& world, f32 dt)
	{
		ZoneScoped;

		const ecs_component_table_t& alive_table			= world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& disabled_table			= world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& destroyer_table		= world.get_component_table(type_id_t<component_destroyer_t>::value);
		ecs_component_table_t&		 system_destroyer_table = world.get_component_table(type_id_t<component_system_destroyer_t>::value);

		// sync destroyers
		{
			const ecs_component_table_ref_t create_refs[] = {
				alive_table.ref(),
				!disabled_table.ref(),
				destroyer_table.ref(),
				!system_destroyer_table.ref(),
			};
			frame_vector_t<entity_id_t> create_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = create_refs, .size = std::size(create_refs)}))
				create_entities.push_back(row.id);

			for (const entity_id_t entity : create_entities)
			{
				const component_destroyer_t& destroyer = destroyer_table.get_as_const<component_destroyer_t>(entity);
				const f32					 end_time  = destroyer.randomize_duration ? destroyer.min_duration + random_t::random_01() * (destroyer.max_duration - destroyer.min_duration) : destroyer.destroy_duration;

				component_system_destroyer_t& system_destroyer = system_destroyer_table.add_or_get_as<component_system_destroyer_t>(entity);

				system_destroyer.timer	  = 0.0f;
				system_destroyer.end_time = end_time;
			}
		}

		{
			const ecs_component_table_ref_t disabled_refs[] = {
				alive_table.ref(),
				system_destroyer_table.ref(),
				disabled_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = disabled_refs, .size = std::size(disabled_refs)}))
				destroy_entities.push_back(row.id);

			for (const entity_id_t entity : destroy_entities)
				system_destroyer_table.remove(entity);
		}

		{
			const ecs_component_table_ref_t missing_refs[] = {
				alive_table.ref(),
				system_destroyer_table.ref(),
				!destroyer_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = missing_refs, .size = std::size(missing_refs)}))
				destroy_entities.push_back(row.id);

			for (const entity_id_t entity : destroy_entities)
				system_destroyer_table.remove(entity);
		}

		// tick destroyers
		const ecs_component_table_ref_t tick_refs[] = {
			alive_table.ref(),
			system_destroyer_table.ref(),
		};
		frame_vector_t<entity_id_t> destroy_entities = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = tick_refs, .size = std::size(tick_refs)}))
		{
			component_system_destroyer_t& system_destroyer = row.get_mutable<component_system_destroyer_t>(1);

			system_destroyer.timer += dt;

			if (system_destroyer.timer >= system_destroyer.end_time)
				destroy_entities.push_back(row.id);
		}

		for (const entity_id_t entity : destroy_entities)
			world.destroy_entity(entity);
	}

	void world_util_t::sync_sprite_renderers(world_t& world)
	{
		ZoneScoped;

		struct sprite_create_t
		{
			component_system_sprite_renderer_t renderer = {};
			entity_id_t						   id		= NULL_ENTITY_ID;
		};

		ecs_component_table_t&			sprite_renderer_table		 = world.get_component_table(type_id_t<component_sprite_renderer_t>::value);
		ecs_component_table_t&			system_sprite_renderer_table = world.get_component_table(type_id_t<component_system_sprite_renderer_t>::value);
		const ecs_component_table_t&	alive_table					 = world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	disabled_table				 = world.get_component_table(type_id_t<component_disabled_t>::value);
		resource_manager_t&				resource_manager			 = resource_manager_t::get();
		frame_vector_t<entity_id_t>		destroy_sprites				 = {};
		frame_vector_t<sprite_create_t> create_sprites				 = {};

		const ecs_component_table_ref_t existing_refs[] = {
			alive_table.ref(),
			sprite_renderer_table.ref(),
			system_sprite_renderer_table.ref(),
			!disabled_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = existing_refs, .size = std::size(existing_refs)}))
		{
			const component_sprite_renderer_t&	sprite		  = row.get<component_sprite_renderer_t>(1);
			component_system_sprite_renderer_t& system_sprite = row.get_mutable<component_system_sprite_renderer_t>(2);
			const resource_entry_t*				sprite_entry  = resource_manager.find_entry(sprite.sprite);

			if (sprite_entry == nullptr || sprite_entry->type != resource_type_e::sprite || sprite_entry->state != resource_state_e::ready)
			{
				destroy_sprites.push_back(row.id);
				continue;
			}

			const sprite_runtime_t*	  sprite_runtime   = resource_manager.find_runtime<sprite_runtime_t>(sprite.sprite);
			const sprite_internals_t* sprite_internals = resource_manager.find_internals<sprite_internals_t>(sprite.sprite);

			if (sprite.row >= sprite_runtime->header.row_count || sprite.column >= sprite_runtime->header.column_count)
			{
				destroy_sprites.push_back(row.id);
				continue;
			}

			const vec2f_t uv_start{
				static_cast<f32>(sprite.column) * sprite_runtime->uv_stride.x,
				static_cast<f32>(sprite.row) * sprite_runtime->uv_stride.y,
			};

			system_sprite = {
				.texture	  = sprite_internals->texture,
				.uv_start	  = uv_start,
				.uv_size	  = sprite_runtime->uv_size,
				.texture_size = sprite_runtime->header.size,
			};
		}

		const ecs_component_table_ref_t create_refs[] = {
			alive_table.ref(),
			sprite_renderer_table.ref(),
			!system_sprite_renderer_table.ref(),
			!disabled_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = create_refs, .size = std::size(create_refs)}))
		{
			const component_sprite_renderer_t& sprite		= row.get<component_sprite_renderer_t>(1);
			const resource_entry_t*			   sprite_entry = resource_manager.find_entry(sprite.sprite);

			if (sprite_entry == nullptr || sprite_entry->type != resource_type_e::sprite || sprite_entry->state != resource_state_e::ready)
				continue;

			const sprite_runtime_t*	  sprite_runtime   = resource_manager.find_runtime<sprite_runtime_t>(sprite.sprite);
			const sprite_internals_t* sprite_internals = resource_manager.find_internals<sprite_internals_t>(sprite.sprite);

			if (sprite.row >= sprite_runtime->header.row_count || sprite.column >= sprite_runtime->header.column_count)
				continue;

			const vec2f_t uv_start{
				static_cast<f32>(sprite.column) * sprite_runtime->uv_stride.x,
				static_cast<f32>(sprite.row) * sprite_runtime->uv_stride.y,
			};

			create_sprites.push_back({
				.renderer =
					{
						.texture	  = sprite_internals->texture,
						.uv_start	  = uv_start,
						.uv_size	  = sprite_runtime->uv_size,
						.texture_size = sprite_runtime->header.size,
					},
				.id = row.id,
			});
		}

		{
			const ecs_component_table_ref_t destroy_refs[] = {
				alive_table.ref(),
				system_sprite_renderer_table.ref(),
				disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = destroy_refs, .size = std::size(destroy_refs)}))
				destroy_sprites.push_back(row.id);
		}

		{
			const ecs_component_table_ref_t destroy_refs[] = {
				alive_table.ref(),
				system_sprite_renderer_table.ref(),
				!sprite_renderer_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = destroy_refs, .size = std::size(destroy_refs)}))
				destroy_sprites.push_back(row.id);
		}

		for (const entity_id_t entity : destroy_sprites)
			system_sprite_renderer_table.remove(entity);

		for (const sprite_create_t& create : create_sprites)
			system_sprite_renderer_table.add_or_get_as<component_system_sprite_renderer_t>(create.id) = create.renderer;
	}

	void world_util_t::sync_reflection_probes(world_t& world, u64 tick_count)
	{
		ZoneScoped;

		const ecs_component_table_t&	alive_table			   = world.get_component_table(type_id_t<component_alive_t>::value);
		ecs_component_table_t&			reflection_probe_table = world.get_component_table(type_id_t<component_reflection_probe_t>::value);
		const ecs_component_table_ref_t table_refs[]		   = {
			alive_table.ref(),
			reflection_probe_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			component_reflection_probe_t& reflection_probe = row.get_mutable<component_reflection_probe_t>(1);

			if (reflection_probe.capture_mode != reflection_probe_capture_mode_e::realtime)
				continue;

			if (tick_count % reflection_probe.realtime_tick_interval == 0)
				++reflection_probe.generation;
		}
	}

	entity_id_t world_util_t::sync_main_camera(world_t& world)
	{
		ZoneScoped;

		const ecs_component_table_t&	alive_table		= world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	camera_table	= world.get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t&	transform_table = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t&	disabled_table	= world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_ref_t table_refs[]	= {
			alive_table.ref(),
			camera_table.ref(),
			transform_table.ref(),
			!disabled_table.ref(),
		};
		entity_id_t main_camera_entity = NULL_ENTITY_ID;
		i8			min_priority	   = 0;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_camera_t& camera = row.get<component_camera_t>(1);

			if (main_camera_entity == NULL_ENTITY_ID || camera.priority < min_priority)
			{
				min_priority	   = camera.priority;
				main_camera_entity = row.id;
			}
		}

		return main_camera_entity;
	}

	void world_util_t::sync_aabbs(world_t& world)
	{
		ZoneScoped;

		const ecs_component_table_t&	alive_table			  = world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	aabb_table			  = world.get_component_table(type_id_t<component_system_aabb_t>::value);
		const ecs_component_table_t&	disabled_table		  = world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t&	particle_table		  = world.get_component_table(type_id_t<component_particle_emitter_t>::value);
		const ecs_component_table_t&	system_particle_table = world.get_component_table(type_id_t<component_system_particle_emitter_t>::value);
		const ecs_component_table_t&	sprite_table		  = world.get_component_table(type_id_t<component_sprite_renderer_t>::value);
		const ecs_component_table_t&	system_sprite_table	  = world.get_component_table(type_id_t<component_system_sprite_renderer_t>::value);
		const ecs_component_table_t&	mesh_table			  = world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t&	skinned_mesh_table	  = world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t&	ragdoll_table		  = world.get_component_table(type_id_t<component_system_ragdoll_t>::value);
		const resource_manager_t&		resources			  = resource_manager_t::get();
		const ecs_component_table_ref_t reset_refs[]		  = {alive_table.ref(), aabb_table.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = reset_refs, .size = std::size(reset_refs)}))
			row.get_mutable<component_system_aabb_t>(1) = {};

		const ecs_component_table_ref_t particle_refs[] = {
			alive_table.ref(),
			aabb_table.ref(),
			system_particle_table.ref(),
			particle_table.ref(),
			!disabled_table.ref(),
			!mesh_table.ref(),
			!skinned_mesh_table.ref(),
			!sprite_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = particle_refs, .size = std::size(particle_refs)}))
		{
			const component_system_particle_emitter_t& particle = row.get<component_system_particle_emitter_t>(2);

			row.get_mutable<component_system_aabb_t>(1) = {.bounds = particle.bounds, .is_world_space = true};
		}

		const ecs_component_table_ref_t sprite_refs[] = {
			alive_table.ref(),
			aabb_table.ref(),
			system_sprite_table.ref(),
			sprite_table.ref(),
			!disabled_table.ref(),
			!particle_table.ref(),
			!mesh_table.ref(),
			!skinned_mesh_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = sprite_refs, .size = std::size(sprite_refs)}))
		{
			const component_system_sprite_renderer_t& sprite	   = row.get<component_system_sprite_renderer_t>(2);
			const f32								  frame_width  = sprite.uv_size.x * static_cast<f32>(sprite.texture_size.x);
			const f32								  frame_height = sprite.uv_size.y * static_cast<f32>(sprite.texture_size.y);
			const f32								  aspect	   = frame_width / frame_height;

			row.get_mutable<component_system_aabb_t>(1).bounds = aabb_t({-aspect * 0.5f, -0.5f, -0.01f}, {aspect * 0.5f, 0.5f, 0.01f});
		}

		const ecs_component_table_ref_t mesh_refs[] = {
			alive_table.ref(),
			aabb_table.ref(),
			mesh_table.ref(),
			!disabled_table.ref(),
			!particle_table.ref(),
			!sprite_table.ref(),
			!skinned_mesh_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = mesh_refs, .size = std::size(mesh_refs)}))
		{
			const component_mesh_renderer_t& renderer = row.get<component_mesh_renderer_t>(2);
			const mesh_internals_t*			 mesh	  = resources.find_internals<mesh_internals_t>(renderer.mesh);

			if (mesh == nullptr)
				continue;

			row.get_mutable<component_system_aabb_t>(1).bounds = mesh->local_bounds;
		}

		const ecs_component_table_ref_t skinned_mesh_refs[] = {
			alive_table.ref(),
			aabb_table.ref(),
			skinned_mesh_table.ref(),
			!disabled_table.ref(),
			!particle_table.ref(),
			!sprite_table.ref(),
			!mesh_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = skinned_mesh_refs, .size = std::size(skinned_mesh_refs)}))
		{
			const component_system_ragdoll_t* ragdoll = ragdoll_table.find_as_const<component_system_ragdoll_t>(row.id);

			if (ragdoll != nullptr)
			{
				row.get_mutable<component_system_aabb_t>(1) = {.bounds = ragdoll->world_bounds, .is_world_space = true};
				continue;
			}

			const component_skinned_mesh_renderer_t& renderer = row.get<component_skinned_mesh_renderer_t>(2);
			const mesh_internals_t*					 mesh	  = resources.find_internals<mesh_internals_t>(renderer.mesh);

			if (mesh == nullptr)
				continue;

			row.get_mutable<component_system_aabb_t>(1).bounds = mesh->local_bounds;
		}
	}
}
