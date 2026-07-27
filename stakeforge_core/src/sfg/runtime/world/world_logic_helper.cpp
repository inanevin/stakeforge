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

#include "world_logic_helper.hpp"
#include "ecs.hpp"
#include "ecs_helpers.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/random.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/sprite.hpp>

#include <tracy/Tracy.hpp>

namespace sfg
{
	void world_logic_helper_t::init(world_t& world)
	{
		SFG_ASSERT(_world == nullptr);

		_world = &world;
	}

	void world_logic_helper_t::uninit()
	{
		SFG_ASSERT(_world != nullptr);

		_world = nullptr;
	}

	void world_logic_helper_t::sync_destroyers(f32 dt)
	{
		ZoneScoped;

		const ecs_component_table_t& alive_table			= _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& disabled_table			= _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& destroyer_table		= _world->get_component_table(type_id_t<component_destroyer_t>::value);
		ecs_component_table_t&		 system_destroyer_table = _world->get_component_table(type_id_t<component_system_destroyer_t>::value);

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
				const component_destroyer_t& destroyer = ecs_helpers_t::table_get_as_const<component_destroyer_t>(destroyer_table, entity);
				const f32					 end_time  = destroyer.randomize_duration ? destroyer.min_duration + random_t::random_01() * (destroyer.max_duration - destroyer.min_duration) : destroyer.destroy_duration;

				component_system_destroyer_t& system_destroyer = ecs_helpers_t::table_add_or_get_as<component_system_destroyer_t>(system_destroyer_table, entity);
				system_destroyer.timer						   = 0.0f;
				system_destroyer.end_time					   = end_time;
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
				ecs_t::table_remove(system_destroyer_table, entity);
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
				ecs_t::table_remove(system_destroyer_table, entity);
		}

		// tick destroyers
		const ecs_component_table_ref_t tick_refs[] = {
			alive_table.ref(),
			system_destroyer_table.ref(),
		};
		frame_vector_t<entity_id_t> destroy_entities = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = tick_refs, .size = std::size(tick_refs)}))
		{
			component_system_destroyer_t& system_destroyer = ecs_helpers_t::row_get_mutable<component_system_destroyer_t>(row, 1);
			system_destroyer.timer += dt;

			if (system_destroyer.timer >= system_destroyer.end_time)
				destroy_entities.push_back(row.id);
		}

		for (const entity_id_t entity : destroy_entities)
			_world->destroy_entity(entity);
	}

	void world_logic_helper_t::sync_sprite_renderers()
	{
		ZoneScoped;

		struct sprite_create_t
		{
			component_system_sprite_renderer_t renderer = {};
			entity_id_t						   id		= NULL_ENTITY_ID;
		};

		ecs_component_table_t&			sprite_renderer_table		 = _world->get_component_table(type_id_t<component_sprite_renderer_t>::value);
		ecs_component_table_t&			system_sprite_renderer_table = _world->get_component_table(type_id_t<component_system_sprite_renderer_t>::value);
		const ecs_component_table_t&	alive_table					 = _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	disabled_table				 = _world->get_component_table(type_id_t<component_disabled_t>::value);
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
			const component_sprite_renderer_t&	sprite		  = ecs_helpers_t::row_get<component_sprite_renderer_t>(row, 1);
			component_system_sprite_renderer_t& system_sprite = ecs_helpers_t::row_get_mutable<component_system_sprite_renderer_t>(row, 2);
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
			const component_sprite_renderer_t& sprite		= ecs_helpers_t::row_get<component_sprite_renderer_t>(row, 1);
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
			ecs_t::table_remove(system_sprite_renderer_table, entity);

		for (const sprite_create_t& create : create_sprites)
			ecs_helpers_t::table_add_or_get_as<component_system_sprite_renderer_t>(system_sprite_renderer_table, create.id) = create.renderer;
	}

	void world_logic_helper_t::sync_reflection_probes(u64 tick_count)
	{
		ZoneScoped;

		const ecs_component_table_t&	alive_table			   = _world->get_component_table(type_id_t<component_alive_t>::value);
		ecs_component_table_t&			reflection_probe_table = _world->get_component_table(type_id_t<component_reflection_probe_t>::value);
		const ecs_component_table_ref_t table_refs[]		   = {
			alive_table.ref(),
			reflection_probe_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			component_reflection_probe_t& reflection_probe = ecs_helpers_t::row_get_mutable<component_reflection_probe_t>(row, 1);

			if (reflection_probe.capture_mode != reflection_probe_capture_mode_e::realtime)
				continue;

			if (tick_count % reflection_probe.realtime_tick_interval == 0)
				++reflection_probe.generation;
		}
	}

	entity_id_t world_logic_helper_t::sync_main_camera()
	{
		ZoneScoped;

		const ecs_component_table_t&	alive_table		= _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	camera_table	= _world->get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t&	transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t&	disabled_table	= _world->get_component_table(type_id_t<component_disabled_t>::value);
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
			const component_camera_t& camera = ecs_helpers_t::row_get<component_camera_t>(row, 1);

			if (main_camera_entity == NULL_ENTITY_ID || camera.priority < min_priority)
			{
				min_priority	   = camera.priority;
				main_camera_entity = row.id;
			}
		}

		return main_camera_entity;
	}
}
