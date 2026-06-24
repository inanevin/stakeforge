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

#include "world_snapshot_producer.hpp"
#include "world.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skybox_hdr.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

#include <iterator>

namespace sfg
{
	void world_snapshot_producer_t::produce(const world_t& world, world_render_snapshot_t& snapshot)
	{
		snapshot.materials.resize(0);
		snapshot.entities.resize(0);
		snapshot.draws.resize(0);
		snapshot.skybox = {};

		const world_component_table_t* transform_table = world.find_component_table(component_system_transform_t::TYPE_ID);
		const world_component_table_t* alive_table	   = world.find_component_table(component_alive_t::TYPE_ID);
		const world_component_table_t* render_table	   = world.find_component_table(component_render_object_t::TYPE_ID);
		const world_component_table_t* camera_table	   = world.find_component_table(component_camera_t::TYPE_ID);
		const world_component_table_t* skybox_table	   = world.find_component_table(component_skybox_t::TYPE_ID);
		const world_component_table_t* disabled_table  = world.find_component_table(component_disabled_t::TYPE_ID);
		SFG_ASSERT(transform_table != nullptr);
		SFG_ASSERT(alive_table != nullptr);
		SFG_ASSERT(render_table != nullptr);
		SFG_ASSERT(camera_table != nullptr);
		SFG_ASSERT(skybox_table != nullptr);
		SFG_ASSERT(disabled_table != nullptr);

		// collect render objects.
		{
			const ecs_component_table_ref_t table_refs[] = {
				transform_table->table.ref(),
				alive_table->table.ref(),
				render_table->table.ref(),
				!disabled_table->table.ref(),
			};

			u32 render_id = 0;
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
				component_render_object_t&			render	  = ecs_helpers_t::row_get_mutable<component_render_object_t>(row, 2);
				render.render_id							  = render_id;

				world_render_entity_t& entity = snapshot.entities.emplace_back();
				entity.prev_transform		  = transform.prev_abs_mat;
				entity.transform			  = transform.abs_mat;
				entity.prev_rot				  = transform.prev_abs_rot;
				entity.rot					  = transform.abs_rot;
				entity.prev_pos				  = transform.prev_abs_pos;
				entity.prev_scale			  = transform.prev_abs_scale;
				entity.pos					  = transform.abs_pos;
				entity.scale				  = transform.abs_scale;
				entity.render_id			  = render_id;

				render_id++;
			}
		}

		// extract main camera
		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table->table.ref(),
				transform_table->table.ref(),
				render_table->table.ref(),
				camera_table->table.ref(),
				!disabled_table->table.ref(),
			};

			i8			min_prio		= 127;
			entity_id_t min_prio_entity = NULL_ENTITY_ID;

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_camera_t&			cam		  = ecs_helpers_t::row_get<component_camera_t>(row, 3);

				if (cam.priority < min_prio)
				{
					min_prio		= cam.priority;
					min_prio_entity = row.id;
				}
			}

			if (min_prio_entity != NULL_ENTITY_ID)
			{
				component_camera_t&			  cam	= ecs_helpers_t::table_get_as<component_camera_t>(camera_table->table, min_prio_entity);
				component_system_transform_t& trans = ecs_helpers_t::table_get_as<component_system_transform_t>(transform_table->table, min_prio_entity);
				snapshot.main_view					= {.pos = trans.abs_pos, .rot = trans.abs_rot, .prev_pos = trans.prev_abs_pos, .prev_rot = trans.prev_abs_rot, .near_plane = cam.near_plane, .far_plane = cam.far_plane, .fov_degrees = cam.fov_degrees};
			}
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table->table.ref(),
				skybox_table->table.ref(),
				!disabled_table->table.ref(),
			};

			const resource_manager_t& resource_manager = resource_manager_t::get();
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_skybox_t& skybox = ecs_helpers_t::row_get<component_skybox_t>(row, 1);
				const resource_entry_t*	  entry	 = resource_manager.find_entry(skybox.skybox_asset);
				if (entry == nullptr || entry->type != resource_type_e::hdr_skybox || entry->state != resource_state_e::ready)
					continue;

				const skybox_hdr_internals_t* internals = resource_manager.get_memory().get<skybox_hdr_internals_t>(entry->internals);
				const skybox_hdr_runtime_t*	  runtime	= resource_manager.get_memory().get<skybox_hdr_runtime_t>(entry->runtime);
				snapshot.skybox							= {
					.radiance	= internals->radiance_texture,
					.irradiance = internals->irradiance_texture,
					.prefilter	= internals->prefilter_texture,
					.brdf_lut	= internals->brdf_lut_texture,
					.intensity	= runtime->intensity * skybox.intensity,
					.exposure	= skybox.exposure,
				};
				break;
			}
		}
	}
}
