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

#include "world_rendering_util.hpp"
#include "render_view.hpp"
#include "world_gpu_bone.hpp"
#include "world_gpu_entity.hpp"
#include "world_gpu_light.hpp"
#include "world_gpu_reflection_probe.hpp"
#include "render_resources.hpp"
#include "world_render_context.hpp"
#include "world_render_reflection_context.hpp"
#include "world_render_snapshot.hpp"
#include <sfg/gfx/util/shadow_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/mat3x3.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	static const vec3f_t CUBEMAP_FACE_DIRECTIONS[WORLD_RENDER_REFLECTION_FACE_COUNT] = {vec3f_t::right, -vec3f_t::right, vec3f_t::up, -vec3f_t::up, vec3f_t::forward, -vec3f_t::forward};
	static const vec3f_t CUBEMAP_FACE_UPS[WORLD_RENDER_REFLECTION_FACE_COUNT]		 = {vec3f_t::up, vec3f_t::up, -vec3f_t::forward, vec3f_t::forward, vec3f_t::up, vec3f_t::up};

	void world_rendering_util_t::prep_entity_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index)
	{
		// prep entity buffer.
		SFG_ASSERT(snapshot.entities.size() <= ctx.get_entity_max());

		gpu_entity_t* entity_buffer = reinterpret_cast<gpu_entity_t*>(ctx.get_mapped_entity_buffer(frame_index));

		for (size_t i = 0; i < snapshot.entities.size(); ++i)
		{
			const world_render_entity_t& entity	 = snapshot.entities[i];
			const vec3f_t				 pos	 = vec3f_t::lerp(entity.prev_pos, entity.pos, interpolation_alpha);
			const quat_t				 rot	 = quat_t::slerp(entity.prev_rot, entity.rot, interpolation_alpha);
			const vec3f_t				 scale	 = vec3f_t::lerp(entity.prev_scale, entity.scale, interpolation_alpha);
			const mat4x4_t				 model	 = mat4x4_t::transform(pos, rot, scale);
			const vec3f_t				 forward = rot.get_forward();

			entity_buffer[i] = {
				.model		   = model,
				.normal_matrix = model.get_normal_matrix(),
				.position	   = vec4f_t(pos.x, pos.y, pos.z, static_cast<f32>(entity.render_id)),
				.forward	   = vec4f_t(forward.x, forward.y, forward.z, 0.0f),
			};
		}
	}

	void world_rendering_util_t::prep_bone_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index)
	{
		// prep bone buffer.
		SFG_ASSERT(snapshot.bones.size() <= ctx.get_bone_max());

		gpu_bone_t* bone_buffer = reinterpret_cast<gpu_bone_t*>(ctx.get_mapped_bone_buffer(frame_index));

		for (size_t i = 0; i < snapshot.bones.size(); ++i)
			bone_buffer[i] = snapshot.bones[i].gpu_bone;
	}

	void world_rendering_util_t::prep_shadows(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha)
	{
		const engine_shadow_settings_t& shadows		   = snapshot.shadows;
		world_render_shadow_context_t&	shadow_context = ctx.get_shadow_context();

		prep_data.shadow_views.resize(0);

		shadow_context.begin_allocations();

		// select quality metrics
		u64 shadow_texels	 = 0;
		f32 quality_scale	 = 1.0f;
		u16 quality_view_max = shadows.max_views;
		switch (snapshot.quality_level)
		{
		case engine_quality_level_e::low:
			quality_scale	 = 0.5f;
			quality_view_max = math::min<u16>(quality_view_max, 12);
			break;
		case engine_quality_level_e::medium:
			quality_scale	 = 0.75f;
			quality_view_max = math::min<u16>(quality_view_max, 24);
			break;
		case engine_quality_level_e::high:
			break;
		case engine_quality_level_e::ultra:
			quality_scale = 2.0f;
			break;
		}

		const u16 shadow_view_max	  = math::min(shadow_context.get_view_max(), quality_view_max);
		const u64 shadow_texel_budget = static_cast<u64>(static_cast<double>(shadows.texel_budget) * quality_scale);
		const f32 shadow_distance	  = shadows.shadow_distance * quality_scale;

		for (u32 light_index = 0; light_index < snapshot.lights.size() && prep_data.shadow_views.size() < shadow_view_max; ++light_index)
		{
			const world_render_light_t& light = snapshot.lights[light_index];

			if (light.cast_shadows == 0 || light.type == static_cast<u8>(world_render_light_type_e::area))
				continue;

			const vec3f_t pos			  = vec3f_t::lerp(light.prev_pos, light.pos, interpolation_alpha);
			const quat_t  rot			  = quat_t::slerp(light.prev_rot, light.rot, interpolation_alpha);
			const f32	  camera_distance = vec3f_t::distance(pos, main_camera_view.pos);

			if (light.type != static_cast<u8>(world_render_light_type_e::directional) && camera_distance - light.range > shadow_distance)
				continue;

			const f32 shadow_fade =
				light.type == static_cast<u8>(world_render_light_type_e::directional) ? 1.0f : 1.0f - math::clamp((camera_distance - light.range - (shadow_distance - shadows.shadow_fade_distance)) / math::max(shadows.shadow_fade_distance, 0.001f), 0.0f, 1.0f);

			u16 resolution = static_cast<u16>(math::clamp<u32>(light.shadow_resolution, shadows.min_resolution, shadows.max_resolution));
			f32 projected  = 0.0f;

			// calc dyn resolution based off distance.
			if (light.type != static_cast<u8>(world_render_light_type_e::directional))
			{
				projected = light.range * static_cast<f32>(ctx.get_size().y) / math::max(camera_distance, 0.1f);

				while (resolution > shadows.min_resolution && projected < static_cast<f32>(resolution) * 0.5f)
					resolution = static_cast<u16>(resolution / 2);

				const world_render_shadow_allocation_t* current = shadow_context.find_allocation(light.stable_id, light.type);

				if (current != nullptr && current->resolution.x >= shadows.min_resolution && current->resolution.x <= shadows.max_resolution)
				{
					if (resolution > current->resolution.x && projected < static_cast<f32>(current->resolution.x) * 0.75f)
						resolution = current->resolution.x;
					else if (resolution < current->resolution.x && projected > static_cast<f32>(current->resolution.x) * 0.35f)
						resolution = current->resolution.x;
				}
			}

			// figure out shadow faces & required texels.
			const u8  layer_count	   = light.type == static_cast<u8>(world_render_light_type_e::point) ? 6 : light.type == static_cast<u8>(world_render_light_type_e::directional) ? math::min<u8>(light.shadow_cascade_count, 8) : 1;
			const u64 requested_texels = static_cast<u64>(resolution) * resolution * layer_count;
			if (shadow_texels + requested_texels > shadow_texel_budget || prep_data.shadow_views.size() + layer_count > shadow_view_max)
				continue;

			world_render_shadow_allocation_t* allocation = shadow_context.get_or_create_allocation(light.stable_id, light.type, {resolution, resolution}, layer_count);
			if (allocation == nullptr)
				continue;

			shadow_texels += requested_texels;

			for (u8 layer = 0; layer < layer_count; ++layer)
			{
				mat4x4_t light_view		= mat4x4_t::identity;
				mat4x4_t light_proj		= mat4x4_t::identity;
				vec3f_t	 light_view_pos = pos;
				f32		 split_near		= light.shadow_near_plane;
				f32		 split_far		= light.range;

				if (light.type == static_cast<u8>(world_render_light_type_e::spot))
				{
					light_view = mat4x4_t::view(rot, pos);
					light_proj = mat4x4_t::perspective(light.outer_cone_degrees * 2.0f, 1.0f, light.shadow_near_plane, light.range);
				}
				else if (light.type == static_cast<u8>(world_render_light_type_e::point))
				{
					light_view = mat4x4_t::look_at(pos, pos + CUBEMAP_FACE_DIRECTIONS[layer], CUBEMAP_FACE_UPS[layer]);
					light_proj = mat4x4_t::perspective(90.0f, 1.0f, light.shadow_near_plane, light.range);
				}
				else
				{
					const f32 n		 = main_camera_view.near_plane;
					const f32 f		 = math::min(main_camera_view.far_plane, shadow_distance);
					const f32 ratio0 = static_cast<f32>(layer) / layer_count;
					const f32 ratio1 = static_cast<f32>(layer + 1) / layer_count;
					split_near		 = math::lerp(n + (f - n) * ratio0, n * std::pow(f / n, ratio0), 0.65f);
					split_far		 = math::lerp(n + (f - n) * ratio1, n * std::pow(f / n, ratio1), 0.65f);

					const mat4x4_t				 cascade_proj = mat4x4_t::perspective_reverse_z(snapshot.main_view.fov_degrees, static_cast<f32>(ctx.get_size().x) / ctx.get_size().y, split_near, split_far);
					inplace_vector_t<vec4f_t, 8> corners	  = {};
					vec3f_t						 center		  = vec3f_t::zero;
					shadow_util_t::get_world_space_ndc((cascade_proj * main_camera_view.view).inverse(), corners, center);

					const vec3f_t forward = rot.get_forward();
					const vec3f_t up	  = math::abs(vec3f_t::dot(forward, vec3f_t::up)) > 0.95f ? vec3f_t::right : vec3f_t::up;
					light_view_pos		  = center - forward * shadow_distance;
					light_view			  = mat4x4_t::look_at(light_view_pos, center, up);
					vec2f_t texel		  = vec2f_t::zero;
					shadow_util_t::get_lightspace_projection(light_proj, light_view, corners, {resolution, resolution}, texel);
				}

				const mat4x4_t view_proj = light_proj * light_view;

				// this for culling
				const world_render_prep_view_t prep_view = {
					.view			   = light_view,
					.view_proj		   = view_proj,
					.inv_view		   = light_view.inverse(),
					.inv_view_proj	   = view_proj.inverse(),
					.frustum		   = frustum_t::extract(view_proj),
					.camera_pos		   = vec4f_t(light_view_pos.x, light_view_pos.y, light_view_pos.z, 1.0f),
					.viewport_size	   = vec2f_t(resolution, resolution),
					.inv_viewport_size = vec2f_t(1.0f / resolution, 1.0f / resolution),
					.near_plane		   = light.shadow_near_plane,
					.far_plane		   = light.range,
				};
				const u16 cull_view_index = prep_data.add_view(prep_view);

				// this for rendering
				prep_data.shadow_views.push_back({.resolution	   = {resolution, resolution},
												  .texture		   = allocation->texture,
												  .texture_index   = allocation->texture_index,
												  .light_index	   = light_index,
												  .split_near	   = split_near,
												  .split_far	   = split_far,
												  .near_plane	   = light.shadow_near_plane,
												  .far_plane	   = light.range,
												  .fade			   = shadow_fade,
												  .cull_view_index = cull_view_index,
												  .view_index	   = layer,
												  .type			   = light.type});
			}
		}

		shadow_context.end_allocations();
	}

	void world_rendering_util_t::prep_shadow_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
		// shadow buffer prep
		const engine_shadow_settings_t& shadows		   = snapshot.shadows;
		world_render_shadow_context_t&	shadow_context = ctx.get_shadow_context();

		for (u32 view_index = 0; view_index < prep_data.shadow_views.size(); ++view_index)
		{
			const world_render_shadow_view_t& view		= prep_data.shadow_views[view_index];
			const world_render_prep_view_t&	  prep_view = prep_data.views[view.cull_view_index];
			const world_render_light_t&		  light		= snapshot.lights[view.light_index];

			// what lighting code uses to shade shadows.
			const gpu_shadow_view_t gpu_view = {
				.view_proj	   = prep_view.view_proj,
				.params0	   = {view.split_near, view.split_far, view.near_plane, view.far_plane},
				.params1	   = {1.0f / view.resolution.x, 1.0f / view.resolution.y, 0.0f, 0.0f},
				.params2	   = {light.shadow_bias, light.shadow_normal_bias, view.fade, shadows.shadow_fade_distance},
				.texture_index = view.texture_index,
				.slice		   = view.view_index,
				.type		   = view.type,
			};
			SFG_MEMCPY(shadow_context.get_mapped_views(frame_index) + view_index * sizeof(gpu_shadow_view_t), &gpu_view, sizeof(gpu_shadow_view_t));

			// what shadow rendering uses as perspective
			const render_pass_data_view_gpu_t shadow_pass_data = {
				.view								 = prep_view.view,
				.view_proj							 = prep_view.view_proj,
				.inv_view							 = prep_view.inv_view,
				.inv_view_proj						 = prep_view.inv_view_proj,
				.camera_pos							 = prep_view.camera_pos,
				.cluster_depth						 = prep_view.cluster_depth,
				.cluster_dims						 = {prep_view.cluster_dims[0], prep_view.cluster_dims[1], prep_view.cluster_dims[2], prep_view.cluster_dims[3]},
				.viewport_size						 = prep_view.viewport_size,
				.inv_viewport_size					 = prep_view.inv_viewport_size,
				.near_plane							 = prep_view.near_plane,
				.far_plane							 = prep_view.far_plane,
				.depth_texture_index				 = prep_view.depth_texture_index,
				.cluster_buffer_offset				 = prep_view.cluster_buffer_offset,
				.cluster_light_indices_buffer_offset = prep_view.cluster_light_indices_buffer_offset,
				.cluster_light_capacity				 = prep_view.cluster_light_capacity,
			};
			SFG_MEMCPY(shadow_context.get_mapped_view_data(frame_index, static_cast<u16>(view_index)), &shadow_pass_data, sizeof(render_pass_data_view_gpu_t));
		}
	}

	void world_rendering_util_t::prep_light_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, u32 (&light_counts)[4])
	{
		// light buffer prep.
		SFG_ASSERT(snapshot.lights.size() <= ctx.get_light_max());

		gpu_light_t* light_buffer = reinterpret_cast<gpu_light_t*>(ctx.get_mapped_light_buffer(frame_index));

		for (size_t i = 0; i < snapshot.lights.size(); ++i)
		{
			const world_render_light_t& light = snapshot.lights[i];
			SFG_ASSERT(light.type < static_cast<u32>(world_render_light_type_e::count));
			++light_counts[light.type];

			const vec3f_t pos	  = vec3f_t::lerp(light.prev_pos, light.pos, interpolation_alpha);
			const quat_t  rot	  = quat_t::slerp(light.prev_rot, light.rot, interpolation_alpha);
			const vec3f_t forward = rot.get_forward();
			const vec3f_t right	  = rot.get_right();
			f32			  param0  = 0.0f;
			f32			  param1  = 0.0f;

			if (light.type == static_cast<u32>(world_render_light_type_e::spot))
			{
				param0 = std::cos(math::degrees_to_radians(light.inner_cone_degrees));
				param1 = std::cos(math::degrees_to_radians(light.outer_cone_degrees));
			}
			else if (light.type == static_cast<u32>(world_render_light_type_e::area))
			{
				param0 = light.area_height * ((light.flags & 1u) != 0 ? -0.5f : 0.5f);
				param1 = light.area_width * 0.5f;
			}

			u32 shadow_offset = UINT32_MAX;
			u32 shadow_count  = 0;

			for (u32 view_index = 0; view_index < prep_data.shadow_views.size(); ++view_index)
			{
				if (prep_data.shadow_views[view_index].light_index != i)
					continue;

				if (shadow_offset == UINT32_MAX)
					shadow_offset = view_index;

				++shadow_count;
			}

			light_buffer[i] = {
				.position_range	  = {pos.x, pos.y, pos.z, light.range},
				.direction_param0 = {forward.x, forward.y, forward.z, param0},
				.right_param1	  = {right.x, right.y, right.z, param1},
				.color_intensity  = {light.color.x, light.color.y, light.color.z, light.intensity},
				.shadow			  = {shadow_offset, shadow_count, 0, 0},
			};
		}
	}

	void world_rendering_util_t::prep_probe(world_render_reflection_context_t&	   reflection_context,
											world_render_reflection_allocation_t&  allocation,
											const world_render_reflection_probe_t& reflection_probe,
											world_render_prep_data_t&			   prep_data,
											f32									   interpolation_alpha,
											u8									   frame_index,
											u32									   cluster_buffer_offset,
											u32									   cluster_light_indices_buffer_offset,
											u16*								   out_cull_view_indices)
	{

		const vec3f_t  pos				 = vec3f_t::lerp(reflection_probe.prev_pos, reflection_probe.pos, interpolation_alpha);
		const quat_t   rot				 = quat_t::slerp(reflection_probe.prev_rot, reflection_probe.rot, interpolation_alpha);
		const vec3f_t  scale			 = vec3f_t::lerp(reflection_probe.prev_scale, reflection_probe.scale, interpolation_alpha);
		const vec3f_t  extents			 = vec3f_t::abs(reflection_probe.extents * scale);
		const f32	   far_plane		 = math::max(extents.magnitude(), reflection_probe.near_plane + 0.01f);
		const u32	   cluster_count_x	 = (static_cast<u32>(allocation.resolution) + WORLD_RENDER_CLUSTER_TILE_SIZE - 1) / WORLD_RENDER_CLUSTER_TILE_SIZE;
		const u32	   cluster_count_y	 = cluster_count_x;
		const u32	   cluster_count	 = cluster_count_x * cluster_count_y * WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT;
		const f32	   cluster_log_scale = WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT / std::log2(far_plane / reflection_probe.near_plane);
		const f32	   cluster_log_bias	 = -std::log2(reflection_probe.near_plane) * cluster_log_scale;
		const mat4x4_t projection		 = mat4x4_t::perspective_reverse_z(90.0f, 1.0f, reflection_probe.near_plane, far_plane);
		const vec2f_t  viewport_size	 = vec2f_t(allocation.resolution, allocation.resolution);
		const vec2f_t  inv_viewport_size = vec2f_t(1.0f / allocation.resolution, 1.0f / allocation.resolution);

		for (u8 face = 0; face < WORLD_RENDER_REFLECTION_FACE_COUNT; ++face)
		{
			const vec3f_t  direction					= rot * CUBEMAP_FACE_DIRECTIONS[face];
			const vec3f_t  up							= rot * CUBEMAP_FACE_UPS[face];
			const mat4x4_t view							= mat4x4_t::look_at(pos, pos + direction, up);
			const mat4x4_t view_proj					= projection * view;
			const u32	   cluster_offset				= cluster_buffer_offset + static_cast<u32>(face) * cluster_count;
			const u32	   cluster_light_indices_offset = cluster_light_indices_buffer_offset + static_cast<u32>(face) * cluster_count * WORLD_RENDER_CLUSTER_LIGHT_CAPACITY;

			// for culling
			const world_render_prep_view_t prep_view = {
				.view								 = view,
				.view_proj							 = view_proj,
				.inv_view							 = view.inverse(),
				.inv_view_proj						 = view_proj.inverse(),
				.frustum							 = frustum_t::extract(view_proj),
				.camera_pos							 = vec4f_t(pos.x, pos.y, pos.z, 1.0f),
				.cluster_depth						 = vec4f_t(reflection_probe.near_plane, far_plane, cluster_log_scale, cluster_log_bias),
				.cluster_dims						 = {cluster_count_x, cluster_count_y, WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT, WORLD_RENDER_CLUSTER_TILE_SIZE},
				.viewport_size						 = viewport_size,
				.inv_viewport_size					 = inv_viewport_size,
				.near_plane							 = reflection_probe.near_plane,
				.far_plane							 = far_plane,
				.depth_texture_index				 = allocation.depth_texture_index,
				.cluster_buffer_offset				 = cluster_offset,
				.cluster_light_indices_buffer_offset = cluster_light_indices_offset,
				.cluster_light_capacity				 = WORLD_RENDER_CLUSTER_LIGHT_CAPACITY,
			};

			if (reflection_probe.capture_type == world_render_reflection_probe_capture_type_e::scene)
				out_cull_view_indices[face] = prep_data.add_view(prep_view);
			else
				out_cull_view_indices[face] = 0;

			// for rendering view
			const render_pass_data_view_gpu_t view_data = {
				.view								 = prep_view.view,
				.view_proj							 = prep_view.view_proj,
				.inv_view							 = prep_view.inv_view,
				.inv_view_proj						 = prep_view.inv_view_proj,
				.camera_pos							 = prep_view.camera_pos,
				.cluster_depth						 = prep_view.cluster_depth,
				.cluster_dims						 = {prep_view.cluster_dims[0], prep_view.cluster_dims[1], prep_view.cluster_dims[2], prep_view.cluster_dims[3]},
				.viewport_size						 = prep_view.viewport_size,
				.inv_viewport_size					 = prep_view.inv_viewport_size,
				.near_plane							 = prep_view.near_plane,
				.far_plane							 = prep_view.far_plane,
				.depth_texture_index				 = prep_view.depth_texture_index,
				.cluster_buffer_offset				 = prep_view.cluster_buffer_offset,
				.cluster_light_indices_buffer_offset = prep_view.cluster_light_indices_buffer_offset,
				.cluster_light_capacity				 = prep_view.cluster_light_capacity,
				.flags								 = 0,
			};
			SFG_MEMCPY(reflection_context.get_mapped_view_data(allocation, frame_index, face), &view_data, sizeof(render_pass_data_view_gpu_t));
		}
	}

	void world_rendering_util_t::prep_probes(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha, u8 frame_index)
	{
		// reflection probe buffer prep.
		world_render_reflection_context_t& reflection_context	   = ctx.get_reflection_context();
		gpu_reflection_probe_t*			   reflection_probe_buffer = reinterpret_cast<gpu_reflection_probe_t*>(reflection_context.get_mapped_probe_buffer(frame_index));

		prep_data.reflection_probe_count = 0;

		reflection_context.begin_allocations();

		for (size_t i = 0; i < snapshot.reflection_probes.size(); ++i)
		{
			const world_render_reflection_probe_t& reflection_probe = snapshot.reflection_probes[i];
			const u16							   resolution		= static_cast<u16>(math::round(reflection_probe.resolution));
			world_render_reflection_allocation_t*  allocation		= reflection_context.get_or_create_allocation(reflection_probe.stable_id, resolution, reflection_probe.generation);

			if (allocation == nullptr || reflection_probe.disabled != 0 || allocation->ready == 0)
				continue;

			const vec3f_t pos	  = vec3f_t::lerp(reflection_probe.prev_pos, reflection_probe.pos, interpolation_alpha);
			const quat_t  rot	  = quat_t::slerp(reflection_probe.prev_rot, reflection_probe.rot, interpolation_alpha);
			const vec3f_t scale	  = vec3f_t::lerp(reflection_probe.prev_scale, reflection_probe.scale, interpolation_alpha);
			const vec3f_t extents = vec3f_t::abs(reflection_probe.extents * scale);

			// if not global, won't be sampled by main camera.
			if (reflection_probe.is_global == 0)
			{
				const vec3f_t influence_extents = extents + vec3f_t::one * reflection_probe.blend_distance;

				if (reflection_probe.max_distance > 0.0f)
				{
					const vec3f_t local_camera_offset = rot.conjugate() * (main_camera_view.pos - pos);
					const vec3f_t bounds_distance	  = vec3f_t::max(vec3f_t::abs(local_camera_offset) - influence_extents, vec3f_t::zero);

					if (bounds_distance.magnitude_sqr() > reflection_probe.max_distance * reflection_probe.max_distance)
						continue;
				}

				const aabb_t   influence_bounds(-influence_extents, influence_extents);
				const mat3x3_t rotation_matrix = mat3x3_t::rotation(rot);

				if (frustum_t::test(main_camera_view.frustum, influence_bounds, rotation_matrix, pos) == frustum_result::outside)
					continue;
			}

			SFG_ASSERT(prep_data.reflection_probe_count < reflection_context.get_probe_max());

			// buffer will be used for sampling
			reflection_probe_buffer[prep_data.reflection_probe_count] = {
				.position_blend_distance	   = {pos.x, pos.y, pos.z, reflection_probe.blend_distance},
				.rotation					   = {rot.x, rot.y, rot.z, rot.w},
				.extents_diffuse_intensity	   = {extents.x, extents.y, extents.z, reflection_probe.diffuse_intensity},
				.specular_intensity			   = reflection_probe.specular_intensity,
				.flags						   = reflection_probe.is_global != 0 ? gpu_reflection_probe_flag_global : 0u,
				.radiance_texture_index		   = allocation->radiance_texture_index,
				.specular_texture_index		   = allocation->specular_texture_index,
				.diffuse_sh_buffer_index	   = reflection_context.get_diffuse_sh_buffer_index(),
				.diffuse_sh_coefficient_offset = allocation->diffuse_sh_coefficient_offset,
				.specular_mip_count			   = allocation->specular_mip_count,
			};
			++prep_data.reflection_probe_count;
		}

		reflection_context.end_allocations();
	}

	world_render_reflection_allocation_t* world_rendering_util_t::prep_reflection_allocation(
		world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, const world_render_reflection_probe_t*& out_reflection_probe, u16* out_cull_view_indices)
	{
		// find he first probe that wants to be rendered.
		world_render_reflection_context_t&	  reflection_context	= ctx.get_reflection_context();
		world_render_reflection_allocation_t* reflection_allocation = nullptr;

		out_reflection_probe = nullptr;

		for (u16 allocation_index = 0; allocation_index < reflection_context.get_probe_max(); ++allocation_index)
		{
			world_render_reflection_allocation_t& candidate = reflection_context.get_allocation(allocation_index);

			if (candidate.pending_render == 0)
				continue;

			for (const world_render_reflection_probe_t& candidate_probe : snapshot.reflection_probes)
			{
				const u16 resolution = static_cast<u16>(math::round(candidate_probe.resolution));

				if (candidate_probe.disabled != 0 || candidate_probe.stable_id != candidate.stable_id || resolution != candidate.resolution || candidate_probe.generation != candidate.generation)
					continue;

				reflection_allocation = &candidate;
				out_reflection_probe  = &candidate_probe;
				break;
			}

			if (reflection_allocation != nullptr)
				break;
		}

		if (reflection_allocation == nullptr)
			return nullptr;

		const u32 main_cluster_count = prep_data.views[0].cluster_dims[0] * prep_data.views[0].cluster_dims[1] * prep_data.views[0].cluster_dims[2];

		prep_probe(reflection_context, *reflection_allocation, *out_reflection_probe, prep_data, interpolation_alpha, frame_index, main_cluster_count, main_cluster_count * WORLD_RENDER_CLUSTER_LIGHT_CAPACITY, out_cull_view_indices);

		reflection_allocation->pending_render = 0;
		reflection_allocation->ready		  = 1;

		return reflection_allocation;
	}

	void world_rendering_util_t::prep_debug_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index)
	{
		// debug copy
		const world_debug_draw_snapshot_t& debug_draw = snapshot.debug_draw;

		if (!debug_draw.line_indices.empty())
		{
			SFG_ASSERT(!debug_draw.line_vertices.empty());

			SFG_MEMCPY(ctx.get_mapped_debug_line_vertices(frame_index), debug_draw.line_vertices.data(), debug_draw.line_vertices.size() * sizeof(vertex_debug_line_t));
			SFG_MEMCPY(ctx.get_mapped_debug_line_indices(frame_index), debug_draw.line_indices.data(), debug_draw.line_indices.size() * sizeof(primitive_index));

			const render_pass_data_debug_line_gpu_t line_data = {
				.params = vec4f_t(0.00005f, 0.0f, 0.0f, 0.0f),
			};

			SFG_MEMCPY(ctx.get_mapped_debug_line_data(frame_index), &line_data, sizeof(render_pass_data_debug_line_gpu_t));
		}

		if (!debug_draw.triangle_indices.empty())
		{
			SFG_ASSERT(!debug_draw.triangle_vertices.empty());

			SFG_MEMCPY(ctx.get_mapped_debug_triangle_vertices(frame_index), debug_draw.triangle_vertices.data(), debug_draw.triangle_vertices.size() * sizeof(vertex_debug_triangle_t));
			SFG_MEMCPY(ctx.get_mapped_debug_triangle_indices(frame_index), debug_draw.triangle_indices.data(), debug_draw.triangle_indices.size() * sizeof(primitive_index));
		}

		if (!debug_draw.text_indices.empty())
		{
			SFG_ASSERT(!debug_draw.text_vertices.empty());

			SFG_MEMCPY(ctx.get_mapped_debug_text_vertices(frame_index), debug_draw.text_vertices.data(), debug_draw.text_vertices.size() * sizeof(vertex_debug_text_t));
			SFG_MEMCPY(ctx.get_mapped_debug_text_indices(frame_index), debug_draw.text_indices.data(), debug_draw.text_indices.size() * sizeof(primitive_index));

			const render_pass_data_debug_text_gpu_t text_data = {
				.params = vec4f_t(0.00005f, 0.0f, 0.0f, 0.0f),
			};

			SFG_MEMCPY(ctx.get_mapped_debug_text_data(frame_index), &text_data, sizeof(render_pass_data_debug_text_gpu_t));
		}
	}

	void world_rendering_util_t::prep_culls(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, u8 frame_index)
	{
		// culling, for every prep data view inserted cull every draw against them.

		gpu_entity_t* entity_buffer = reinterpret_cast<gpu_entity_t*>(ctx.get_mapped_entity_buffer(frame_index));
		const u32	  draw_count	= static_cast<u32>(snapshot.draws.size());
		const u32	  word_count	= (draw_count + 63) / 64;
		const size_t  mask_count	= prep_data.views.size() * word_count;

		prep_data.cull_word_count = word_count;
		prep_data.draw_cull_masks.resize(mask_count);

		if (mask_count != 0)
			SFG_MEMSET(prep_data.draw_cull_masks.data(), 0, mask_count * sizeof(u64));

		for (u32 draw_index = 0; draw_index < draw_count; ++draw_index)
		{
			const world_draw_t& draw = snapshot.draws[draw_index];

			if (draw.aabb.is_empty())
				continue;

			SFG_ASSERT(draw.entity_index < snapshot.entities.size());

			const mat4x4_t& model = entity_buffer[draw.entity_index].model;
			const mat3x3_t	linear_model(model[0], model[1], model[2], model[4], model[5], model[6], model[8], model[9], model[10]);
			const vec3f_t	position = model.get_translation();

			for (size_t view_index = 0; view_index < prep_data.views.size(); ++view_index)
			{
				if (frustum_t::test(prep_data.views[view_index].frustum, draw.aabb, linear_model, position) != frustum_result::outside)
					continue;

				u64& mask_word = prep_data.draw_cull_masks[view_index * word_count + draw_index / 64];
				mask_word |= 1ull << (draw_index % 64);
			}
		}
	}

	void world_rendering_util_t::prep_render_pass_buffers(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, const u32 (&light_counts)[4], u8 frame_index)
	{
		// rp data
		const world_render_prep_view_t&	  main_prep_view		= prep_data.views[0];
		const render_pass_data_view_gpu_t view_render_pass_data = {
			.view								 = main_prep_view.view,
			.view_proj							 = main_prep_view.view_proj,
			.inv_view							 = main_prep_view.inv_view,
			.inv_view_proj						 = main_prep_view.inv_view_proj,
			.camera_pos							 = main_prep_view.camera_pos,
			.cluster_depth						 = main_prep_view.cluster_depth,
			.cluster_dims						 = {main_prep_view.cluster_dims[0], main_prep_view.cluster_dims[1], main_prep_view.cluster_dims[2], main_prep_view.cluster_dims[3]},
			.viewport_size						 = main_prep_view.viewport_size,
			.inv_viewport_size					 = main_prep_view.inv_viewport_size,
			.near_plane							 = main_prep_view.near_plane,
			.far_plane							 = main_prep_view.far_plane,
			.depth_texture_index				 = main_prep_view.depth_texture_index,
			.cluster_buffer_offset				 = main_prep_view.cluster_buffer_offset,
			.cluster_light_indices_buffer_offset = main_prep_view.cluster_light_indices_buffer_offset,
			.cluster_light_capacity				 = main_prep_view.cluster_light_capacity,
		};

		SFG_MEMCPY(ctx.get_mapped_view_render_pass_data(frame_index), &view_render_pass_data, sizeof(render_pass_data_view_gpu_t));

		const render_resources_t&			  render_resources			= render_resources_t::get();
		const render_pass_data_lighting_gpu_t lighting_render_pass_data = {
			.ambient_color							= snapshot.environment.ambient_color,
			.light_counts							= {light_counts[0], light_counts[1], light_counts[2], light_counts[3]},
			.light_buffer_index						= ctx.get_light_buffer_index(frame_index),
			.shadow_buffer_index					= ctx.get_shadow_context().get_view_buffer_index(frame_index),
			.reflection_probe_buffer_index			= ctx.get_reflection_context().get_probe_buffer_index(frame_index),
			.cluster_buffer_index					= ctx.get_light_cluster_buffer_index(frame_index),
			.cluster_buffer_uav_index				= ctx.get_light_cluster_buffer_uav_index(frame_index),
			.cluster_light_indices_buffer_index		= ctx.get_light_cluster_indices_buffer_index(frame_index),
			.cluster_light_indices_buffer_uav_index = ctx.get_light_cluster_indices_buffer_uav_index(frame_index),
			.reflection_probe_count					= prep_data.reflection_probe_count,
			.environment_intensity					= snapshot.environment.intensity,
			.brdf_lut_index							= render_resources.get_texture_gpu_index(render_resources.get_brdf_lut(), 0),
			.debug_cluster_heatmap					= snapshot.environment.debug_cluster_heatmap,
		};

		SFG_MEMCPY(ctx.get_mapped_lighting_render_pass_data(frame_index), &lighting_render_pass_data, sizeof(render_pass_data_lighting_gpu_t));

		const bool									   ssao_active						  = ctx.is_ssao_enabled() && snapshot.post_process.ssao.enabled != 0;
		const gpu_index_t							   ambient_occlusion_index			  = ssao_active ? ctx.get_ao_texture_index(frame_index) : render_resources.get_texture_gpu_index(render_resources.get_white_texture(), 0);
		const render_pass_data_deferred_lighting_gpu_t deferred_lighting_render_pass_data = {
			.gbuffer_albedo_index	 = ctx.get_gbuffer_albedo_index(frame_index),
			.gbuffer_normal_index	 = ctx.get_gbuffer_normal_index(frame_index),
			.gbuffer_orm_index		 = ctx.get_gbuffer_orm_index(frame_index),
			.gbuffer_emissive_index	 = ctx.get_gbuffer_emissive_index(frame_index),
			.ambient_occlusion_index = ambient_occlusion_index,
		};

		SFG_MEMCPY(ctx.get_mapped_deferred_lighting_render_pass_data(frame_index), &deferred_lighting_render_pass_data, sizeof(render_pass_data_deferred_lighting_gpu_t));

		if (ctx.is_ssao_enabled())
		{
			const vec2u16_t size		= ctx.get_size();
			const u32		half_width	= static_cast<u32>(size.x) / 2;
			const u32		half_height = static_cast<u32>(size.y) / 2;

			const render_pass_data_ssao_gpu_t ssao_render_pass_data = {
				.proj				 = main_camera_view.proj,
				.inv_proj			 = main_camera_view.inv_proj,
				.view_matrix		 = main_camera_view.view,
				.full_size			 = {size.x, size.y},
				.half_size			 = {half_width, half_height},
				.inv_full			 = {1.0f / static_cast<f32>(size.x), 1.0f / static_cast<f32>(size.y)},
				.inv_half			 = {1.0f / (static_cast<f32>(size.x) * 0.5f), 1.0f / (static_cast<f32>(size.y) * 0.5f)},
				.z_near				 = main_camera_view.near_plane,
				.z_far				 = main_camera_view.far_plane,
				.radius_world		 = snapshot.post_process.ssao.radius_world,
				.bias				 = snapshot.post_process.ssao.bias,
				.intensity			 = snapshot.post_process.ssao.intensity,
				.power				 = snapshot.post_process.ssao.power,
				.num_dirs			 = snapshot.post_process.ssao.direction_count,
				.num_steps			 = snapshot.post_process.ssao.step_count,
				.random_rot_strength = snapshot.post_process.ssao.random_rotation_strength,
			};

			SFG_MEMCPY(ctx.get_mapped_ssao_render_pass_data(frame_index), &ssao_render_pass_data, sizeof(render_pass_data_ssao_gpu_t));
		}

		if (ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0)
		{
			const render_pass_data_bloom_gpu_t bloom_render_pass_data = {
				.filter_radius = snapshot.post_process.bloom.filter_radius,
			};

			SFG_MEMCPY(ctx.get_mapped_bloom_render_pass_data(frame_index), &bloom_render_pass_data, sizeof(render_pass_data_bloom_gpu_t));
		}
	}
}
