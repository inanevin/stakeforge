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
#include <sfg/runtime/animation/animation_bone.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/resources/cubemap.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/material.hpp>
#include <sfg/runtime/resources/texture_sampler.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

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
				if (mat_entry == nullptr || mat_entry->type != resource_type_e::material)
					return UINT32_MAX;

				material_runtime_t*	  mat_runtime	= rm_aux.get<material_runtime_t>(mat_entry->runtime);
				material_internals_t* mat_internals = rm_aux.get<material_internals_t>(mat_entry->internals);

				const shader_internals_t* shader = rm.find_internals<shader_internals_t>(mat_runtime->shader_guid);
				if (shader == nullptr)
					return UINT32_MAX;

				for (u32 i = 0; i < mat_runtime->texture_count; ++i)
				{
					if (mat_runtime->texture_types[i] == shader_texture_type_e::texture_cube)
					{
						const resource_entry_t* cubemap_entry = rm.find_entry(mat_runtime->texture_guids[i]);

						if (cubemap_entry == nullptr || cubemap_entry->type != resource_type_e::cubemap || cubemap_entry->internals.size == 0)
							return UINT32_MAX;
					}
				}

				const u32 idx					   = static_cast<u32>(snapshot.materials.size());
				material_guid_to_index[mat_handle] = idx;

				snapshot.materials.push_back({});
				world_render_material_t& render_mat = snapshot.materials.back();
				render_mat.pass_mask				= mat_runtime->pass_flags.value();
				render_mat.double_sided				= mat_runtime->double_sided;
				render_mat.use_alpha_cutoff			= mat_runtime->use_alpha_cutoff;
				render_mat.texture_count			= mat_runtime->texture_count;
				render_mat.pso_count				= shader->pso_count;

				for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
					render_mat.material_buffers[frame_index] = mat_internals->parameter_buffers[frame_index];

				for (u32 i = 0; i < shader->pso_count; i++)
				{
					render_mat.psos[i]		= shader->psos[i];
					render_mat.pso_flags[i] = shader->pso_flags[i].value();
				}

				for (u32 j = 0; j < mat_runtime->texture_count; j++)
				{
					switch (mat_runtime->texture_types[j])
					{
					case shader_texture_type_e::texture_cube: {
						const resource_entry_t*	   cubemap_entry	 = rm.find_entry(mat_runtime->texture_guids[j]);
						const cubemap_internals_t* cubemap_internals = rm_aux.get<cubemap_internals_t>(cubemap_entry->internals);
						render_mat.material_textures[j]				 = cubemap_internals->texture;
						break;
					}
					default: {
						const texture_internals_t* texture_internals = rm.find_internals<texture_internals_t>(mat_runtime->texture_guids[j]);
						render_mat.material_textures[j]				 = texture_internals != nullptr ? texture_internals->texture : render_resources_t::get().get_invalid_texture();
						break;
					}
					}

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

	void world_snapshot_producer_t::produce(world_t& world, world_render_snapshot_t& snapshot, const project_settings_t& project_settings)
	{
		resource_manager_t::get().flush_material_updates();

		snapshot.shadows	   = project_settings.shadows;
		snapshot.quality_level = project_settings.quality_level;

		snapshot.materials.resize(0);
		snapshot.entities.resize(0);
		snapshot.bones.resize(0);
		snapshot.lights.resize(0);
		snapshot.reflection_probes.resize(0);
		snapshot.draws.resize(0);
		snapshot.environment  = {};
		snapshot.fog		  = {};
		snapshot.post_process = {};

		world.get_debug_draw().write_snapshot(snapshot.debug_draw);

		const ecs_component_table_t& transform_table					= world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& alive_table						= world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& camera_table						= world.get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t& post_process_table					= world.get_component_table(type_id_t<component_post_process_t>::value);
		const ecs_component_table_t& environment_table					= world.get_component_table(type_id_t<component_environment_t>::value);
		const ecs_component_table_t& fog_table							= world.get_component_table(type_id_t<component_fog_t>::value);
		const ecs_component_table_t& light_table						= world.get_component_table(type_id_t<component_light_t>::value);
		const ecs_component_table_t& reflection_probe_table				= world.get_component_table(type_id_t<component_reflection_probe_t>::value);
		const ecs_component_table_t& disabled_table						= world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& mesh_renderer_table				= world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t& skinned_mesh_renderer_table		= world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t& system_skinned_mesh_renderer_table = world.get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);

		const resource_manager_t&  rm	  = resource_manager_t::get();
		const chunk_allocator32_t& rm_aux = rm.get_memory();

		frame_hash_map_t<entity_id_t, u32>		 entity_to_render_id	= {};
		frame_hash_map_t<resource_handle_t, u32> material_guid_to_index = {};

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

		// extract environment
		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				environment_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_environment_t& environment = ecs_helpers_t::row_get<component_environment_t>(row, 1);
				snapshot.environment.intensity			   = environment.intensity;
				snapshot.environment.ambient_color		   = environment.ambient_color.to_vector();
				snapshot.environment.debug_cluster_heatmap = environment.debug_cluster_heatmap;

				const resource_entry_t* material_entry = rm.find_entry(environment.skybox_material);

				if (material_entry == nullptr || material_entry->type != resource_type_e::material)
					break;

				const material_runtime_t* material_runtime = rm.get_memory().get<material_runtime_t>(material_entry->runtime);
				const resource_entry_t*	  shader_entry	   = rm.find_entry(material_runtime->shader_guid);

				if (shader_entry == nullptr || shader_entry->type != resource_type_e::shader)
					break;

				const shader_runtime_t* shader_runtime = rm.get_memory().get<shader_runtime_t>(shader_entry->runtime);

				if (shader_runtime->type != shader_type_e::skybox_shader)
					break;

				const u32 material_index = push_material_from_guid(material_guid_to_index, snapshot, environment.skybox_material);

				if (material_index == UINT32_MAX)
					break;

				snapshot.environment.material_index = material_index;
				break;
			}
		}

		// extract fog
		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				fog_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_fog_t& fog = ecs_helpers_t::row_get<component_fog_t>(row, 1);

				snapshot.fog = {
					.color			= fog.color.to_vector(),
					.intensity		= fog.intensity,
					.density		= fog.density,
					.start_distance = fog.start_distance,
					.end_distance	= fog.end_distance,
					.height			= fog.height,
					.height_falloff = fog.height_falloff,
					.max_opacity	= fog.max_opacity,
					.type			= static_cast<world_render_fog_type_e>(fog.type),
				};
				break;
			}
		}

		// lights.
		{
			const ecs_component_table_ref_t table_refs[] = {
				transform_table.ref(),
				alive_table.ref(),
				light_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
				const component_light_t&			light	  = ecs_helpers_t::row_get<component_light_t>(row, 2);

				snapshot.lights.push_back({
					.prev_rot			  = transform.prev_abs_rot,
					.rot				  = transform.abs_rot,
					.prev_pos			  = transform.prev_abs_pos,
					.intensity			  = light.intensity,
					.pos				  = transform.abs_pos,
					.range				  = light.range,
					.color				  = {light.color.x, light.color.y, light.color.z},
					.inner_cone_degrees	  = light.inner_cone_degrees,
					.outer_cone_degrees	  = light.outer_cone_degrees,
					.area_width			  = light.area_size.x,
					.area_height		  = light.area_size.y,
					.shadow_near_plane	  = light.shadow_near_plane,
					.shadow_bias		  = light.shadow_bias,
					.shadow_normal_bias	  = light.shadow_normal_bias,
					.stable_id			  = row.id,
					.shadow_resolution	  = light.shadow_resolution,
					.type				  = static_cast<u8>(light.type),
					.flags				  = light.two_sided != 0 ? static_cast<u8>(1) : static_cast<u8>(0),
					.shadow_cascade_count = light.shadow_cascade_count,
					.cast_shadows		  = light.cast_shadows,
				});
			}
		}

		// reflection probes.
		{
			const ecs_component_table_ref_t table_refs[] = {
				transform_table.ref(),
				alive_table.ref(),
				reflection_probe_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform		 = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
				const component_reflection_probe_t& reflection_probe = ecs_helpers_t::row_get<component_reflection_probe_t>(row, 2);

				snapshot.reflection_probes.push_back({
					.prev_rot				= transform.prev_abs_rot,
					.rot					= transform.abs_rot,
					.prev_pos				= transform.prev_abs_pos,
					.diffuse_intensity		= reflection_probe.diffuse_intensity,
					.pos					= transform.abs_pos,
					.specular_intensity		= reflection_probe.specular_intensity,
					.prev_scale				= transform.prev_abs_scale,
					.blend_distance			= reflection_probe.blend_distance,
					.scale					= transform.abs_scale,
					.resolution				= reflection_probe.resolution,
					.extents				= reflection_probe.extents,
					.max_distance			= reflection_probe.max_distance,
					.near_plane				= reflection_probe.near_plane,
					.stable_id				= row.id,
					.generation				= reflection_probe.generation,
					.realtime_tick_interval = reflection_probe.realtime_tick_interval,
					.capture_type			= static_cast<world_render_reflection_probe_capture_type_e>(reflection_probe.capture_type),
					.capture_mode			= static_cast<world_render_reflection_probe_capture_mode_e>(reflection_probe.capture_mode),
					.is_global				= reflection_probe.is_global ? static_cast<u8>(1) : static_cast<u8>(0),
					.disabled				= ecs_t::table_has(disabled_table, row.id) ? static_cast<u8>(1) : static_cast<u8>(0),
				});
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

		// extract skinned mesh renderers
		{
			const ecs_component_table_ref_t table_refs[] = {
				transform_table.ref(),
				alive_table.ref(),
				skinned_mesh_renderer_table.ref(),
				system_skinned_mesh_renderer_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t&				transform					 = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
				const component_skinned_mesh_renderer_t&		skinned_mesh_renderer		 = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 2);
				const component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::row_get<component_system_skinned_mesh_renderer_t>(row, 3);

				if (skinned_mesh_renderer.mesh == NULL_RESOURCE_HANDLE || skinned_mesh_renderer.skeleton == NULL_RESOURCE_HANDLE)
					continue;

				if (!system_skinned_mesh_renderer.bones_handle)
					continue;

				const resource_entry_t* entry = rm.find_entry(skinned_mesh_renderer.mesh);
				if (entry == nullptr)
					continue;

				const mesh_runtime_t*			mesh_runtime   = rm_aux.get<mesh_runtime_t>(entry->runtime);
				const mesh_internals_t*			mesh_internals = rm_aux.get<mesh_internals_t>(entry->internals);
				const mesh_primitive_runtime_t* primitives	   = rm_aux.get<mesh_primitive_runtime_t>(mesh_runtime->primitives);

				const render_resource_handle_t		 vertex_buffer = mesh_internals->vertex_buffer;
				const render_resource_handle_t		 index_buffer  = mesh_internals->index_buffer;
				const u32							 idx		   = static_cast<u32>(snapshot.bones.size());
				const span_t<const animation_bone_t> bones		   = world.get_animation_controller().get_bones(system_skinned_mesh_renderer.bones_handle);

				for (size_t i = 0; i < bones.size; ++i)
				{
					const animation_bone_t& bone = bones.data[i];
					snapshot.bones.push_back({
						.gpu_bone =
							{
								.model = bone.bone_transform.to_matrix4x4(),
							},
					});
				}

				for (u32 i = 0; i < mesh_runtime->primitive_count; i++)
				{
					const mesh_primitive_runtime_t& prim = primitives[i];

					if (skinned_mesh_renderer.materials.size() <= prim.material_index)
						continue;

					const resource_handle_t mat_handle = skinned_mesh_renderer.materials[prim.material_index];

					const u32 draw_material_index = push_material_from_guid(material_guid_to_index, snapshot, mat_handle);
					if (draw_material_index == UINT32_MAX)
						continue;

					const u32 entity_index = push_render_object(entity_to_render_id, snapshot, row.id, transform_table);

					world_draw_t& draw	= snapshot.draws.emplace_back();
					draw.aabb			= mesh_internals->local_bounds;
					draw.sort_key		= mat_handle;
					draw.vertex_buffer	= vertex_buffer;
					draw.index_buffer	= index_buffer;
					draw.pass_mask		= snapshot.materials[draw_material_index].pass_mask;
					draw.draw_flags		= 0;
					draw.material_index = draw_material_index;
					draw.entity_index	= entity_index;
					draw.skinning_index = idx;
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

		std::sort(snapshot.lights.begin(), snapshot.lights.end(), [](const world_render_light_t& l0, const world_render_light_t& l1) -> bool { return l0.type < l1.type; });
		std::sort(snapshot.draws.begin(), snapshot.draws.end(), [](const world_draw_t& d0, const world_draw_t& d1) -> bool { return d0.sort_key < d1.sort_key; });
	}
}
