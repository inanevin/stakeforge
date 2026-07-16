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
#include "world_debug_draw.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/data/frame_hash_map.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skybox_hdr.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/material.hpp>
#include <sfg/runtime/resources/texture_sampler.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

#include <iterator>
#include <algorithm>
namespace sfg
{
	namespace
	{
		u32 push_material_from_guid(frame_hash_map_t<resource_handle_t, u32>& material_guid_to_index, world_render_snapshot_t& snapshot, resource_handle_t mat_handle)
		{
			const resource_manager_t&  rm	  = resource_manager_t::get();
			const chunk_allocator32_t& rm_aux = rm.get_memory();

			auto it = material_guid_to_index.find(mat_handle);
			if (it != material_guid_to_index.end())
			{
				return it->second;
			}
			else
			{
				const resource_entry_t* mat_entry = rm.find_entry(mat_handle);
				if (mat_entry == nullptr)
					return UINT32_MAX;

				material_runtime_t*	  mat_runtime	= rm_aux.get<material_runtime_t>(mat_entry->runtime);
				material_internals_t* mat_internals = rm_aux.get<material_internals_t>(mat_entry->internals);

				const shader_internals_t* shader = rm.find_internals<shader_internals_t>(mat_runtime->shader_guid);
				if (shader == nullptr)
					return UINT32_MAX;

				const u32 idx					   = static_cast<u32>(snapshot.materials.size());
				material_guid_to_index[mat_handle] = idx;

				snapshot.materials.push_back({});
				world_render_material_t& render_mat = snapshot.materials.back();
				render_mat.pass_mask				= mat_runtime->pass_flags.value();
				render_mat.double_sided				= mat_runtime->double_sided;
				render_mat.use_alpha_cutoff			= mat_runtime->use_alpha_cutoff;
				render_mat.texture_count			= mat_runtime->texture_count;
				render_mat.material_buffer			= mat_internals->parameter_buffer;
				render_mat.pso_count				= shader->pso_count;

				for (u32 i = 0; i < shader->pso_count; i++)
				{
					render_mat.psos[i]		= shader->psos[i];
					render_mat.pso_flags[i] = shader->pso_flags[i].value();
				}

				for (u32 j = 0; j < mat_runtime->texture_count; j++)
				{
					const texture_internals_t* texture_internals = rm.find_internals<texture_internals_t>(mat_runtime->texture_guids[j]);
					render_mat.material_textures[j]				 = texture_internals ? texture_internals->texture : render_resources_t::get().get_invalid_texture();

					render_mat.material_samplers[j] = render_resources_t::get().get_default_linear_sampler();
					if (mat_runtime->sampler_count != 0)
					{
						const u32						   sampler_index	 = j < mat_runtime->sampler_count ? j : mat_runtime->sampler_count - 1;
						const texture_sampler_internals_t* sampler_internals = rm.find_internals<texture_sampler_internals_t>(mat_runtime->sampler_guids[sampler_index]);
						render_mat.material_samplers[j]						 = sampler_internals ? sampler_internals->sampler : render_resources_t::get().get_default_linear_sampler();
					}
				}

				return idx;
			}
		}

		u32 push_render_object(frame_hash_map_t<entity_id_t, u32>& entity_to_render_id, world_render_snapshot_t& snapshot, entity_id_t id, const ecs_component_table_t& transform_table)
		{
			auto it = entity_to_render_id.find(id);
			if (it != entity_to_render_id.end())
				return it->second;

			const u32 emplaced_id	= static_cast<u32>(snapshot.entities.size());
			entity_to_render_id[id] = emplaced_id;

			const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, id);

			world_render_entity_t& entity = snapshot.entities.emplace_back();
			entity.entity_id			  = id;
			entity.prev_transform		  = transform.prev_abs_mat;
			entity.transform			  = transform.abs_mat;
			entity.prev_rot				  = transform.prev_abs_rot;
			entity.rot					  = transform.abs_rot;
			entity.prev_pos				  = transform.prev_abs_pos;
			entity.prev_scale			  = transform.prev_abs_scale;
			entity.pos					  = transform.abs_pos;
			entity.scale				  = transform.abs_scale;

			return emplaced_id;
		}
	}

	void world_snapshot_producer_t::produce(world_t& world, world_render_snapshot_t& snapshot)
	{
		snapshot.materials.resize(0);
		snapshot.entities.resize(0);
		snapshot.draws.resize(0);
		snapshot.skybox		  = {};
		snapshot.post_process = {};
		world.get_debug_draw().write_snapshot(snapshot.debug_draw);

		const ecs_component_table_t& transform_table	 = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& alive_table		 = world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& camera_table		 = world.get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t& post_process_table	 = world.get_component_table(type_id_t<component_post_process_t>::value);
		const ecs_component_table_t& skybox_table		 = world.get_component_table(type_id_t<component_skybox_t>::value);
		const ecs_component_table_t& disabled_table		 = world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& mesh_renderer_table = world.get_component_table(type_id_t<component_mesh_renderer_t>::value);

		const resource_manager_t&  rm	  = resource_manager_t::get();
		const chunk_allocator32_t& rm_aux = rm.get_memory();

		frame_hash_map_t<entity_id_t, u32>		 entity_to_render_id;
		frame_hash_map_t<resource_handle_t, u32> material_guid_to_index;

		// extract main camera
		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				transform_table.ref(),
				camera_table.ref(),
				!disabled_table.ref(),
			};

			i8			min_prio		= 127;
			entity_id_t min_prio_entity = NULL_ENTITY_ID;

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_camera_t&			cam		  = ecs_helpers_t::row_get<component_camera_t>(row, 2);

				if (cam.priority < min_prio)
				{
					min_prio		= cam.priority;
					min_prio_entity = row.id;
				}
			}

			if (min_prio_entity != NULL_ENTITY_ID)
			{
				component_camera_t&			  cam	= ecs_helpers_t::table_get_as<component_camera_t>(camera_table, min_prio_entity);
				component_system_transform_t& trans = ecs_helpers_t::table_get_as<component_system_transform_t>(transform_table, min_prio_entity);
				snapshot.main_view					= {.pos = trans.abs_pos, .rot = trans.abs_rot, .prev_pos = trans.prev_abs_pos, .prev_rot = trans.prev_abs_rot, .near_plane = cam.near_plane, .far_plane = cam.far_plane, .fov_degrees = cam.fov_degrees};

				const component_post_process_t* post_process = ecs_helpers_t::table_find_as_const<component_post_process_t>(post_process_table, min_prio_entity);
				if (post_process != nullptr)
				{
					snapshot.post_process = {
						.ssao =
							{
								.radius_world			  = post_process->ssao.radius_world,
								.bias					  = post_process->ssao.bias,
								.intensity				  = post_process->ssao.intensity,
								.power					  = post_process->ssao.power,
								.random_rotation_strength = post_process->ssao.random_rotation_strength,
								.direction_count		  = post_process->ssao.direction_count,
								.step_count				  = post_process->ssao.step_count,
								.enabled				  = post_process->ssao.enabled,
							},
						.bloom =
							{
								.strength	   = post_process->bloom.strength,
								.filter_radius = post_process->bloom.filter_radius,
								.enabled	   = post_process->bloom.enabled,
							},
						.exposure_ev		  = post_process->exposure_ev,
						.saturation			  = post_process->saturation,
						.temperature		  = post_process->temperature,
						.tint				  = post_process->tint,
						.reinhard_white_point = post_process->reinhard_white_point,
						.tonemap_mode		  = static_cast<u32>(post_process->tonemap_mode),
					};
				}
			}
		}

		// extract skybox
		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				skybox_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_skybox_t& skybox = ecs_helpers_t::row_get<component_skybox_t>(row, 1);
				const resource_entry_t*	  entry	 = rm.find_entry(skybox.skybox_asset);
				if (entry == nullptr || entry->type != resource_type_e::hdr_skybox)
					continue;

				const skybox_hdr_internals_t* internals = rm.get_memory().get<skybox_hdr_internals_t>(entry->internals);
				const skybox_hdr_runtime_t*	  runtime	= rm.get_memory().get<skybox_hdr_runtime_t>(entry->runtime);
				snapshot.skybox							= {
					.radiance		   = internals->radiance_texture,
					.irradiance		   = internals->irradiance_texture,
					.prefilter		   = internals->prefilter_texture,
					.brdf_lut		   = internals->brdf_lut_texture,
					.intensity		   = runtime->intensity * skybox.intensity,
					.exposure		   = skybox.exposure,
					.rotation		   = runtime->rotation,
					.prefilter_max_lod = static_cast<f32>(runtime->prefilter.mip_count - 1),
				};
				break;
			}
		}

		// extract mesh renderers
		{
			const ecs_component_table_ref_t table_refs[] = {
				transform_table.ref(),
				alive_table.ref(),
				mesh_renderer_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform	  = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
				const component_mesh_renderer_t&	mesh_renderer = ecs_helpers_t::row_get<component_mesh_renderer_t>(row, 2);
				if (mesh_renderer.mesh == NULL_RESOURCE_HANDLE)
					continue;

				const resource_entry_t* entry = rm.find_entry(mesh_renderer.mesh);
				if (entry == nullptr)
					continue;

				const mesh_runtime_t*			mesh_runtime   = rm_aux.get<mesh_runtime_t>(entry->runtime);
				const mesh_internals_t*			mesh_internals = rm_aux.get<mesh_internals_t>(entry->internals);
				const mesh_primitive_runtime_t* primitives	   = rm_aux.get<mesh_primitive_runtime_t>(mesh_runtime->primitives);

				const render_resource_handle_t vtx = mesh_internals->vertex_buffer;
				const render_resource_handle_t idx = mesh_internals->index_buffer;

				for (u32 i = 0; i < mesh_runtime->primitive_count; i++)
				{
					const mesh_primitive_runtime_t& prim = primitives[i];

					if (mesh_renderer.materials.size() <= prim.material_index)
						continue;

					const resource_handle_t mat_handle = mesh_renderer.materials[prim.material_index];

					const u32 draw_material_index = push_material_from_guid(material_guid_to_index, snapshot, mat_handle);
					if (draw_material_index == UINT32_MAX)
						continue;

					const u32 entity_index = push_render_object(entity_to_render_id, snapshot, row.id, transform_table);

					world_draw_t& draw	= snapshot.draws.emplace_back();
					draw.aabb			= mesh_internals->local_bounds;
					draw.sort_key		= mat_handle;
					draw.vertex_buffer	= vtx;
					draw.index_buffer	= idx;
					draw.pass_mask		= snapshot.materials[draw_material_index].pass_mask;
					draw.draw_flags		= 0;
					draw.material_index = draw_material_index;
					draw.entity_index	= entity_index;
					draw.skinning_index = UINT32_MAX;
					draw.index_count	= prim.index_count;
					draw.vertex_count	= UINT32_MAX;
					draw.start_index	= prim.start_index;
					draw.start_vertex	= prim.start_vertex;
					draw.start_instance = 0;
					draw.vertex_stride	= mesh_runtime->vertex_stride;
					draw.index_stride	= mesh_runtime->index_stride;
				}
			}
		}

		std::sort(snapshot.draws.begin(), snapshot.draws.end(), [](const world_draw_t& d0, const world_draw_t& d1) -> bool { return d0.sort_key < d1.sort_key; });
	}
}
