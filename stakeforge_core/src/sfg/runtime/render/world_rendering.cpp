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

#include "world_rendering.hpp"
#include "render_view.hpp"
#include "render_resources.hpp"
#include "render_globals.hpp"
#include "world_render_context.hpp"
#include "world_render_snapshot.hpp"
#include "world_rendering_util.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
	namespace
	{
		void bind_vertex(gfx_backend& backend, gfx_handle_t cmd, gfx_handle_t& bound_vertex, gfx_handle_t handle, u16 vtx_size)
		{
			if (handle == bound_vertex)
				return;

			bound_vertex = handle;
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = bound_vertex, .slot = 0, .vertex_size = vtx_size, .offset = 0});
		};

		void bind_index(gfx_backend& backend, gfx_handle_t cmd, gfx_handle_t& bound_index, gfx_handle_t handle, u8 idx_size)
		{
			if (handle == bound_index)
				return;

			bound_index = handle;
			backend.cmd_bind_index_buffers(cmd, {.buffer = bound_index, .offset = 0, .index_size = idx_size});
		};

		void bind_pipeline(gfx_backend& backend, gfx_handle_t cmd, gfx_handle_t& bound_pipeline, gfx_handle_t pipeline)
		{
			if (pipeline == bound_pipeline)
				return;

			bound_pipeline = pipeline;
			backend.cmd_bind_pipeline(cmd, {.pipeline = pipeline});
		}

		void bind_material(gfx_backend& backend, gfx_handle_t cmd, u32& bound_index, u32 index, const world_render_material_t& mat, u8 frame_index)
		{
			if (bound_index == index)
				return;

			bound_index = index;

			inplace_vector_t<gpu_index_t, 1 + SFG_MATERIAL_MAX_TEXTURES * 2> constants		 = {};
			const render_resource_handle_t									 material_buffer = mat.material_buffers[frame_index];
			constants.push_back(material_buffer.is_null() ? NULL_GPU_INDEX : render_resources_t::get().get_resource_gpu_index(material_buffer));

			for (u32 i = 0; i < mat.texture_count; i++)
				constants.push_back(render_resources_t::get().get_texture_gpu_index(mat.material_textures[i], 0));

			for (u32 i = 0; i < mat.texture_count; i++)
				constants.push_back(render_resources_t::get().get_sampler_gpu_index(mat.material_samplers[i]));

			backend.cmd_bind_constants(cmd, {.data = constants.data(), .offset = constant_mat0, .count = static_cast<u8>(constants.size()), .param_index = 0});
		}

		void draw_world_draws(gfx_backend& backend, gfx_handle_t cmd, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u16 cull_view_index, u32 pass_mask, u32 initial_variant_flags, u8 frame_index)
		{
			render_resources_t& render_resources = render_resources_t::get();
			gfx_handle_t		bound_vertex	 = {};
			gfx_handle_t		bound_index		 = {};
			gfx_handle_t		bound_pipeline	 = {};
			u32					bound_material	 = UINT32_MAX;
			const u32			draw_count		 = static_cast<u32>(snapshot.draws.size());

			for (u32 i = 0; i < draw_count; ++i)
			{
				const world_draw_t& draw = snapshot.draws[i];

				if (prep_data.is_draw_culled(cull_view_index, i))
					continue;

				if (pass_mask != 0 && (draw.pass_mask & pass_mask) == 0)
					continue;

				const gfx_handle_t vertex_buffer = render_resources.get_resource(draw.vertex_buffer);
				const gfx_handle_t index_buffer	 = render_resources.get_resource(draw.index_buffer);

				SFG_ASSERT(!vertex_buffer.is_null() && !index_buffer.is_null());

				bind_vertex(backend, cmd, bound_vertex, vertex_buffer, draw.vertex_stride);
				bind_index(backend, cmd, bound_index, index_buffer, draw.index_stride);

				if (!draw.direct_pso.is_null())
				{
					bind_pipeline(backend, cmd, bound_pipeline, render_resources.get_shader_hw(draw.direct_pso));
				}
				else
				{
					SFG_ASSERT(draw.material_index != UINT32_MAX);

					const world_render_material_t& material		 = snapshot.materials[draw.material_index];
					bitmask_t<u32>				   variant_flags = initial_variant_flags;

					if (material.use_alpha_cutoff != 0)
						variant_flags.set(shader_variant_flags_alpha_cutoff);

					if (material.double_sided != 0)
						variant_flags.set(shader_variant_flags_double_sided);

					if (draw.skinning_index != UINT32_MAX)
						variant_flags.set(shader_variant_flags_skinned);

					const render_resource_handle_t shader_handle = material.find_pso(variant_flags);

					if (shader_handle.is_null())
						continue;

					bind_pipeline(backend, cmd, bound_pipeline, render_resources.get_shader_hw(shader_handle));
					bind_material(backend, cmd, bound_material, draw.material_index, material, frame_index);
				}

				const u32 obj_constants[2] = {
					draw.entity_index,
					draw.skinning_index,
				};
				backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
				backend.cmd_draw_indexed_instanced(cmd,
												   {
													   .index_count_per_instance = draw.index_count,
													   .instance_count			 = 1,
													   .start_index_location	 = draw.start_index,
													   .base_vertex_location	 = draw.start_vertex,
													   .start_instance_location	 = 0,
												   });
			}
		}
	}

	void world_rendering_t::render_world(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		gfx_backend&				   backend		  = gfx_backend::get();
		world_render_shadow_context_t& shadow_context = ctx.get_shadow_context();

		world_rendering_util_t::prep_entity_buffer(ctx, snapshot, interpolation_alpha, frame_index);
		world_rendering_util_t::prep_bone_buffer(ctx, snapshot, frame_index);

		// main cam view.
		const vec2u16_t render_size		   = ctx.get_size();
		render_view_t	main_camera_view_t = {};

		main_camera_view_t.calculate(snapshot.main_view, render_size, interpolation_alpha);

		const f32 cluster_log_scale = WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT / std::log2(main_camera_view_t.far_plane / main_camera_view_t.near_plane);
		const f32 cluster_log_bias	= -std::log2(main_camera_view_t.near_plane) * cluster_log_scale;
		u32		  light_counts[4]	= {};

		prep_data.add_view({
			.view								 = main_camera_view_t.view,
			.view_proj							 = main_camera_view_t.view_proj,
			.inv_view							 = main_camera_view_t.inv_view,
			.inv_view_proj						 = main_camera_view_t.inv_view_proj,
			.frustum							 = main_camera_view_t.frustum,
			.camera_pos							 = vec4f_t(main_camera_view_t.pos.x, main_camera_view_t.pos.y, main_camera_view_t.pos.z, 1.0f),
			.cluster_depth						 = vec4f_t(main_camera_view_t.near_plane, main_camera_view_t.far_plane, cluster_log_scale, cluster_log_bias),
			.cluster_dims						 = {ctx.get_light_cluster_count_x(), ctx.get_light_cluster_count_y(), WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT, WORLD_RENDER_CLUSTER_TILE_SIZE},
			.viewport_size						 = vec2f_t(render_size.x, render_size.y),
			.inv_viewport_size					 = vec2f_t(1.0f / render_size.x, 1.0f / render_size.y),
			.near_plane							 = main_camera_view_t.near_plane,
			.far_plane							 = main_camera_view_t.far_plane,
			.depth_texture_index				 = ctx.get_depth_texture_index(frame_index),
			.cluster_buffer_offset				 = 0,
			.cluster_light_indices_buffer_offset = 0,
			.cluster_light_capacity				 = WORLD_RENDER_CLUSTER_LIGHT_CAPACITY,
		});

		world_rendering_util_t::prep_shadows(ctx, snapshot, prep_data, main_camera_view_t, interpolation_alpha);
		world_rendering_util_t::prep_light_buffer(ctx, snapshot, prep_data, interpolation_alpha, frame_index, light_counts);
		world_rendering_util_t::prep_probes(ctx, snapshot, prep_data, main_camera_view_t, interpolation_alpha, frame_index);
		world_rendering_util_t::prep_shadow_buffer(ctx, snapshot, prep_data, frame_index);
		world_rendering_util_t::prep_debug_buffer(ctx, snapshot, frame_index);
		world_rendering_util_t::prep_culls(ctx, snapshot, prep_data, frame_index);
		world_rendering_util_t::prep_render_pass_buffers(ctx, snapshot, prep_data, main_camera_view_t, light_counts, frame_index);

		const gfx_handle_t cmd_depth			  = ctx.get_command_buffer_depth(frame_index);
		const gfx_handle_t cmd_gbuffer			  = ctx.get_command_buffer_gbuffer(frame_index);
		const gfx_handle_t cmd_lighting			  = ctx.get_command_buffer_lighting(frame_index);
		const gfx_handle_t cmd_forward			  = ctx.get_command_buffer_forward(frame_index);
		const gfx_handle_t cmd_post				  = ctx.get_command_buffer_post(frame_index);
		const gfx_handle_t cmd_shadows			  = shadow_context.get_command_buffer(frame_index);
		const gfx_handle_t cmd_clustered_lighting = ctx.get_command_buffer_clustered_lighting(frame_index);
		const bool		   ssao_active			  = ctx.is_ssao_enabled() && snapshot.post_process.ssao.enabled != 0;
		const bool		   bloom_active			  = ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;

		const gfx_handle_t queue_gfx	 = backend.get_queue_gfx();
		const gfx_handle_t queue_compute = backend.get_queue_compute();
		job_system_t&	   jobs			 = job_system_t::get();
		static job_graph_t render_graph;

		render_graph.clear();
		render_graph.emplace([&]() {
			render_access_scope_t render_scope = {};

			render_depth_prepass(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);

			if (!prep_data.shadow_views.empty())
				render_shadows(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		});
		render_graph.emplace([&]() {
			render_access_scope_t render_scope = {};

			render_gbuffer(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		});
		render_graph.emplace([&]() {
			render_access_scope_t render_scope = {};

			render_clustered_lighting({
				.command_buffer				  = ctx.get_command_buffer_clustered_lighting(frame_index),
				.cluster_buffer				  = ctx.get_light_cluster_buffer(frame_index),
				.cluster_light_indices_buffer = ctx.get_light_cluster_indices_buffer(frame_index),
				.shader						  = ctx.get_clustered_light_culling_shader(),
				.view_data_index			  = ctx.get_view_render_pass_data_index(frame_index),
				.lighting_data_index		  = ctx.get_lighting_render_pass_data_index(frame_index),
				.global_cbv_index			  = global_cbv_index,
				.cluster_count_x			  = ctx.get_light_cluster_count_x(),
				.cluster_count_y			  = ctx.get_light_cluster_count_y(),
				.cluster_count_z			  = WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT,
			});
		});

		if (ssao_active)
		{
			render_graph.emplace([&]() {
				render_access_scope_t render_scope = {};
				render_ssao(ctx, snapshot, prep_data, frame_index, global_cbv_index);
			});
		}

		jobs.run(render_graph).wait();

		const gfx_handle_t depth_gbuffer_commands[2] = {cmd_depth, cmd_gbuffer};
		backend.submit_commands(queue_gfx, depth_gbuffer_commands, 2);

		const gfx_handle_t clustered_lighting_semaphore = ctx.get_clustered_lighting_semaphore(frame_index);
		const u64		   clustered_lighting_ready		= ctx.next_clustered_lighting_semaphore_value(frame_index);
		backend.submit_commands(queue_compute, &cmd_clustered_lighting, 1);
		backend.queue_signal(queue_compute, &clustered_lighting_semaphore, &clustered_lighting_ready, 1);

		gfx_handle_t ssao_semaphore = {};
		u64			 gbuffer_ready	= 0;
		u64			 ssao_ready		= 0;

		if (ssao_active)
		{
			ssao_semaphore				= ctx.get_ssao_semaphore(frame_index);
			gbuffer_ready				= ctx.next_ssao_semaphore_value(frame_index);
			ssao_ready					= ctx.next_ssao_semaphore_value(frame_index);
			const gfx_handle_t cmd_ssao = ctx.get_command_buffer_ssao(frame_index);
			backend.queue_signal(queue_gfx, &ssao_semaphore, &gbuffer_ready, 1);
			backend.queue_wait(queue_compute, &ssao_semaphore, &gbuffer_ready, 1);
			backend.submit_commands(queue_compute, &cmd_ssao, 1);
			backend.queue_signal(queue_compute, &ssao_semaphore, &ssao_ready, 1);
		}

		if (!prep_data.shadow_views.empty())
			backend.submit_commands(queue_gfx, &cmd_shadows, 1);

		render_graph.clear();
		render_graph.emplace([&]() {
			render_access_scope_t render_scope = {};

			render_lighting(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
			render_forward(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		});

		jobs.run(render_graph).wait();

		if (ssao_active)
			backend.queue_wait(queue_gfx, &ssao_semaphore, &ssao_ready, 1);

		backend.queue_wait(queue_gfx, &clustered_lighting_semaphore, &clustered_lighting_ready, 1);

		const gfx_handle_t lighting_forward_commands[2] = {cmd_lighting, cmd_forward};
		backend.submit_commands(queue_gfx, lighting_forward_commands, 2);

		gfx_handle_t bloom_semaphore = {};
		u64			 lighting_ready	 = 0;
		u64			 bloom_ready	 = 0;

		if (bloom_active)
		{
			bloom_semaphore = ctx.get_bloom_semaphore(frame_index);
			lighting_ready	= ctx.next_bloom_semaphore_value(frame_index);
			bloom_ready		= ctx.next_bloom_semaphore_value(frame_index);
			backend.queue_signal(queue_gfx, &bloom_semaphore, &lighting_ready, 1);
		}

		render_graph.clear();
		render_graph.emplace([&]() {
			render_access_scope_t render_scope = {};

			render_post_process(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		});

		if (bloom_active)
		{
			render_graph.emplace([&]() {
				render_access_scope_t render_scope = {};
				render_bloom(ctx, snapshot, prep_data, frame_index, global_cbv_index);
			});
		}

		jobs.run(render_graph).wait();

		if (bloom_active)
		{
			const gfx_handle_t cmd_bloom = ctx.get_command_buffer_bloom(frame_index);
			backend.queue_wait(queue_compute, &bloom_semaphore, &lighting_ready, 1);
			backend.submit_commands(queue_compute, &cmd_bloom, 1);
			backend.queue_signal(queue_compute, &bloom_semaphore, &bloom_ready, 1);
			backend.queue_wait(queue_gfx, &bloom_semaphore, &bloom_ready, 1);
		}

		backend.submit_commands(queue_gfx, &cmd_post, 1);
	}

	void world_rendering_t::render_shadows(const world_render_context_t& ctx, const world_render_snapshot_t& ss, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend&						 backend		= gfx_backend::get();
		const world_render_shadow_context_t& shadow_context = ctx.get_shadow_context();
		const gfx_handle_t					 cmd			= shadow_context.get_command_buffer(frame_index);

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		inplace_vector_t<barrier_t, 32> barriers = {};

		for (const world_render_shadow_view_t& view : prep_data.shadow_views)
		{
			if (view.view_index != 0)
				continue;

			barriers.push_back({.from_states = resource_state_ps_resource, .to_states = resource_state_depth_write, .texture_t = view.texture, .flags = barrier_flags::baf_is_texture});
		}

		backend.cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		for (u32 view_index = 0; view_index < prep_data.shadow_views.size(); ++view_index)
		{
			const world_render_shadow_view_t& view = prep_data.shadow_views[view_index];
			backend.cmd_begin_render_pass_depth_only(cmd, {.depth_stencil_attachment = {.texture = view.texture, .clear_depth = 1.0f, .depth_load_op = load_op::clear, .depth_store_op = store_op::store, .view_index = view.view_index}});
			backend.cmd_set_viewport(cmd, {.min_depth = 0.0f, .max_depth = 1.0f, .width = view.resolution.x, .height = view.resolution.y});
			backend.cmd_set_scissors(cmd, {.width = view.resolution.x, .height = view.resolution.y});

			const gpu_index_t rp_constants[3] = {shadow_context.get_view_data_index(frame_index, static_cast<u16>(view_index)), ctx.get_entity_buffer_index(frame_index), ctx.get_bone_buffer_index(frame_index)};

			backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 3, .param_index = 0});

			draw_world_draws(backend, cmd, ss, prep_data, view.cull_view_index, world_pass_flags_shadow, shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering, frame_index);

			backend.cmd_end_render_pass(cmd, {});
		}

		barriers.resize(0);

		for (const world_render_shadow_view_t& view : prep_data.shadow_views)
		{
			if (view.view_index != 0)
				continue;

			barriers.push_back({.from_states = resource_state_depth_write, .to_states = resource_state_ps_resource, .texture_t = view.texture, .flags = barrier_flags::baf_is_texture});
		}

		backend.cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_depth_prepass(const world_render_context_t& ctx, const world_render_snapshot_t& ss, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd			 = ctx.get_command_buffer_depth(frame_index);
		const gfx_handle_t depth_texture = ctx.get_depth_texture(frame_index);
		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const barrier_t begin_barrier = {
			.from_states = resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource,
			.to_states	 = resource_state_depth_write,
			.texture_t	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});

		BEGIN_DEBUG_EVENT((&backend), cmd, "world_depth_prepass");
		backend.cmd_begin_render_pass_depth_only(cmd,
												 {
													 .depth_stencil_attachment =
														 {
															 .texture		 = depth_texture,
															 .clear_stencil	 = 0,
															 .clear_depth	 = 0.0f,
															 .depth_load_op	 = load_op::clear,
															 .depth_store_op = store_op::store,
															 .view_index	 = 0,
														 },
												 });

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		gpu_index_t rp_constants[3] = {ctx.get_view_render_pass_data_index(frame_index), ctx.get_entity_buffer_index(frame_index), ctx.get_bone_buffer_index(frame_index)};

		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 3, .param_index = 0});

		draw_world_draws(backend, cmd, ss, prep_data, 0, world_pass_flags_depth, shader_variant_flags_z_prepass, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t depth_read_barrier = {
			.from_states = resource_state_depth_write,
			.to_states	 = resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource,
			.texture_t	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &depth_read_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_gbuffer(const world_render_context_t& ctx, const world_render_snapshot_t& ss, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				= ctx.get_command_buffer_gbuffer(frame_index);
		const gfx_handle_t depth_texture	= ctx.get_depth_texture(frame_index);
		const gfx_handle_t gbuffer_albedo	= ctx.get_gbuffer_albedo_texture(frame_index);
		const gfx_handle_t gbuffer_normal	= ctx.get_gbuffer_normal_texture(frame_index);
		const gfx_handle_t gbuffer_orm		= ctx.get_gbuffer_orm_texture(frame_index);
		const gfx_handle_t gbuffer_emissive = ctx.get_gbuffer_emissive_texture(frame_index);
		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const gfx_handle_t gbuffer_textures[4] = {gbuffer_albedo, gbuffer_normal, gbuffer_orm, gbuffer_emissive};
		const barrier_t	   begin_barriers[4]   = {
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_textures[0],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_non_ps_resource | resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_textures[1],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_textures[2],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_textures[3],
				.flags		 = barrier_flags::baf_is_texture,
			},
		};
		backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = 4});

		const render_pass_color_attachment_t color_attachments[4] = {
			{
				.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
				.texture	 = gbuffer_albedo,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			},
			{
				.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
				.texture	 = gbuffer_normal,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			},
			{
				.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
				.texture	 = gbuffer_orm,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			},
			{
				.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
				.texture	 = gbuffer_emissive,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			},
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "world_gbuffer");
		backend.cmd_begin_render_pass_depth_read_only(cmd,
													  {
														  .color_attachments = color_attachments,
														  .depth_stencil_attachment =
															  {
																  .texture			= depth_texture,
																  .clear_stencil	= 0,
																  .clear_depth		= 0.0f,
																  .depth_load_op	= load_op::load,
																  .stencil_load_op	= load_op::none,
																  .depth_store_op	= store_op::store,
																  .stencil_store_op = store_op::none,
																  .view_index		= 1,
															  },
														  .color_attachment_count = 4,
													  });

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		gpu_index_t rp_constants[3] = {ctx.get_view_render_pass_data_index(frame_index), ctx.get_entity_buffer_index(frame_index), ctx.get_bone_buffer_index(frame_index)};

		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 3, .param_index = 0});

		draw_world_draws(backend, cmd, ss, prep_data, 0, world_pass_flags_gbuffer, shader_variant_flags_gbuffer, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barriers[4] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = gbuffer_albedo,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_non_ps_resource | resource_state_ps_resource,
				.texture_t	 = gbuffer_normal,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = gbuffer_orm,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = gbuffer_emissive,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};
		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = 4});
		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_clustered_lighting(const world_render_clustered_lighting_data_t& data)
	{
		gfx_backend& backend = gfx_backend::get();

		backend.reset_command_buffer(data.command_buffer);
		backend.cmd_bind_layout_compute(data.command_buffer, {.layout = render_globals_t::get_global_compute_bind_layout()});
		backend.cmd_bind_constants_compute(data.command_buffer, {.data = &data.global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const barrier_t begin_barriers[2] = {
			{
				.from_states = resource_state_common,
				.to_states	 = resource_state_uav,
				.resource_t	 = data.cluster_buffer,
				.flags		 = barrier_flags::baf_is_resource,
			},
			{
				.from_states = resource_state_common,
				.to_states	 = resource_state_uav,
				.resource_t	 = data.cluster_light_indices_buffer,
				.flags		 = barrier_flags::baf_is_resource,
			},
		};
		backend.cmd_barrier(data.command_buffer, {.barriers = begin_barriers, .barrier_count = 2});

		backend.cmd_bind_constants_compute(data.command_buffer, {.data = &data.view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants_compute(data.command_buffer, {.data = &data.lighting_data_index, .offset = constant_rp3, .count = 1, .param_index = 0});
		backend.cmd_bind_pipeline_compute(data.command_buffer, {.pipeline = data.shader});

		BEGIN_DEBUG_EVENT((&backend), data.command_buffer, "world_clustered_light_culling");
		backend.cmd_dispatch(data.command_buffer,
							 {
								 .group_size_x = (data.cluster_count_x + 3) / 4,
								 .group_size_y = (data.cluster_count_y + 3) / 4,
								 .group_size_z = data.cluster_count_z,
							 });
		END_DEBUG_EVENT((&backend), data.command_buffer);

		const barrier_t end_barriers[2] = {
			{
				.from_states = resource_state_uav,
				.to_states	 = resource_state_common,
				.resource_t	 = data.cluster_buffer,
				.flags		 = barrier_flags::baf_is_resource,
			},
			{
				.from_states = resource_state_uav,
				.to_states	 = resource_state_common,
				.resource_t	 = data.cluster_light_indices_buffer,
				.flags		 = barrier_flags::baf_is_resource,
			},
		};
		backend.cmd_barrier(data.command_buffer, {.barriers = end_barriers, .barrier_count = 2});
		backend.close_command_buffer(data.command_buffer);
	}

	void world_rendering_t::render_ssao(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index)
	{
		gfx_backend&	   backend = gfx_backend::get();
		const gfx_handle_t cmd	   = ctx.get_command_buffer_ssao(frame_index);
		const gfx_handle_t ao_half = ctx.get_ao_half_texture(frame_index);
		const gfx_handle_t ao_full = ctx.get_ao_texture(frame_index);
		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout_compute(cmd, {.layout = render_globals_t::get_global_compute_bind_layout()});
		backend.cmd_bind_constants_compute(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		barrier_t begin_barriers[2] = {};
		begin_barriers[0]			= {
			.from_states = resource_state_non_ps_resource,
			.to_states	 = resource_state_uav,
			.texture_t	 = ao_half,
			.flags		 = barrier_flags::baf_is_texture,
		};
		begin_barriers[1] = {
			.from_states = resource_state_non_ps_resource,
			.to_states	 = resource_state_uav,
			.texture_t	 = ao_full,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = 2});

		const vec2u16_t	  size				= ctx.get_size();
		const u32		  half_width		= static_cast<u32>(size.x) / 2;
		const u32		  half_height		= static_cast<u32>(size.y) / 2;
		const gpu_index_t ssao_constants[5] = {
			ctx.get_ssao_render_pass_data_index(frame_index),
			ctx.get_depth_texture_index(frame_index),
			ctx.get_gbuffer_normal_index(frame_index),
			ctx.get_ssao_noise_texture_index(),
			ctx.get_ao_half_texture_uav_index(frame_index),
		};

		backend.cmd_bind_constants_compute(cmd, {.data = ssao_constants, .offset = constant_rp0, .count = 5, .param_index = 0});
		backend.cmd_bind_pipeline_compute(cmd, {.pipeline = ctx.get_ssao_shader()});
		BEGIN_DEBUG_EVENT((&backend), cmd, "world_ssao");
		backend.cmd_dispatch(cmd, {.group_size_x = (half_width + 7) / 8, .group_size_y = (half_height + 7) / 8, .group_size_z = 1});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t half_read_barrier = {
			.from_states = resource_state_uav,
			.to_states	 = resource_state_non_ps_resource,
			.texture_t	 = ao_half,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &half_read_barrier, .barrier_count = 1});

		const gpu_index_t upsample_constants[5] = {
			ctx.get_ssao_render_pass_data_index(frame_index),
			ctx.get_depth_texture_index(frame_index),
			ctx.get_gbuffer_normal_index(frame_index),
			ctx.get_ao_half_texture_index(frame_index),
			ctx.get_ao_texture_uav_index(frame_index),
		};

		backend.cmd_bind_constants_compute(cmd, {.data = upsample_constants, .offset = constant_rp0, .count = 5, .param_index = 0});
		backend.cmd_bind_pipeline_compute(cmd, {.pipeline = ctx.get_ssao_upsample_shader()});
		BEGIN_DEBUG_EVENT((&backend), cmd, "world_ssao_upsample");
		backend.cmd_dispatch(cmd, {.group_size_x = (static_cast<u32>(size.x) + 7) / 8, .group_size_y = (static_cast<u32>(size.y) + 7) / 8, .group_size_z = 1});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t full_read_barrier = {
			.from_states = resource_state_uav,
			.to_states	 = resource_state_non_ps_resource,
			.texture_t	 = ao_full,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &full_read_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_lighting(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				= ctx.get_command_buffer_lighting(frame_index);
		const gfx_handle_t lighting_texture = ctx.get_lighting_texture(frame_index);
		const bool		   ssao_active		= ctx.is_ssao_enabled() && snapshot.post_process.ssao.enabled != 0;
		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		barrier_t begin_barriers[4] = {
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture_t	 = lighting_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};
		u16 begin_count = 1;

		begin_barriers[begin_count++] = {
			.from_states = resource_state_common,
			.to_states	 = resource_state_ps_resource,
			.resource_t	 = ctx.get_light_cluster_buffer(frame_index),
			.flags		 = barrier_flags::baf_is_resource,
		};
		begin_barriers[begin_count++] = {
			.from_states = resource_state_common,
			.to_states	 = resource_state_ps_resource,
			.resource_t	 = ctx.get_light_cluster_indices_buffer(frame_index),
			.flags		 = barrier_flags::baf_is_resource,
		};

		if (ssao_active)
		{
			begin_barriers[begin_count++] = {
				.from_states = resource_state_non_ps_resource,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = ctx.get_ao_texture(frame_index),
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = lighting_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 1,
		};
		BEGIN_DEBUG_EVENT((&backend), cmd, "world_lighting");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		const gpu_index_t view_data_index		= ctx.get_view_render_pass_data_index(frame_index);
		const gpu_index_t lighting_constants[2] = {
			ctx.get_lighting_render_pass_data_index(frame_index),
			ctx.get_deferred_lighting_render_pass_data_index(frame_index),
		};

		backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = lighting_constants, .offset = constant_rp3, .count = 2, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_lighting_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		if (ssao_active)
		{
			const barrier_t end_barrier = {
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_non_ps_resource,
				.texture_t	 = ctx.get_ao_texture(frame_index),
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
		}

		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_forward(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend&		backend			 = gfx_backend::get();
		render_resources_t& render_resources = render_resources_t::get();
		const gfx_handle_t	cmd				 = ctx.get_command_buffer_forward(frame_index);
		const bool			bloom_active	 = ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const render_pass_color_attachment_t color_attachment = {
			.texture	= ctx.get_lighting_texture(frame_index),
			.load_op	= load_op::load,
			.store_op	= store_op::store,
			.view_index = 1,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "world_forward");
		backend.cmd_begin_render_pass_depth_read_only(cmd,
													  {
														  .color_attachments = &color_attachment,
														  .depth_stencil_attachment =
															  {
																  .texture			= ctx.get_depth_texture(frame_index),
																  .clear_stencil	= 0,
																  .clear_depth		= 0.0f,
																  .depth_load_op	= load_op::load,
																  .stencil_load_op	= load_op::none,
																  .depth_store_op	= store_op::store,
																  .stencil_store_op = store_op::none,
																  .view_index		= 1,
															  },
														  .color_attachment_count = 1,
													  });

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		const gpu_index_t rp_constants[4] = {
			ctx.get_view_render_pass_data_index(frame_index),
			ctx.get_entity_buffer_index(frame_index),
			ctx.get_bone_buffer_index(frame_index),
			ctx.get_lighting_render_pass_data_index(frame_index),
		};

		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 4, .param_index = 0});

		if (snapshot.environment.material_index != UINT32_MAX)
		{
			BEGIN_DEBUG_EVENT((&backend), cmd, "world_skybox");

			const world_render_material_t& material		 = snapshot.materials[snapshot.environment.material_index];
			const render_resource_handle_t shader_handle = material.find_pso(0);
			SFG_ASSERT(!shader_handle.is_null());

			backend.cmd_bind_pipeline(cmd, {.pipeline = render_resources.get_shader_hw(shader_handle)});

			u32 bound_material = UINT32_MAX;
			bind_material(backend, cmd, bound_material, snapshot.environment.material_index, material, frame_index);
			backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 36, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
			END_DEBUG_EVENT((&backend), cmd);
		}

		draw_world_draws(backend, cmd, snapshot, prep_data, 0, world_pass_flags_forward, 0, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barriers[3] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = bloom_active ? resource_state_non_ps_resource : resource_state_ps_resource,
				.texture_t	 = ctx.get_lighting_texture(frame_index),
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_common,
				.resource_t	 = ctx.get_light_cluster_buffer(frame_index),
				.flags		 = barrier_flags::baf_is_resource,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_common,
				.resource_t	 = ctx.get_light_cluster_indices_buffer(frame_index),
				.flags		 = barrier_flags::baf_is_resource,
			},
		};

		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = 3});
		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_bloom(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index)
	{
		gfx_backend&	   backend			  = gfx_backend::get();
		const gfx_handle_t cmd				  = ctx.get_command_buffer_bloom(frame_index);
		const vec2u16_t	   size				  = ctx.get_size();
		const gfx_handle_t downsample_texture = ctx.get_bloom_downsample_texture(frame_index);
		const gfx_handle_t upsample_texture	  = ctx.get_bloom_upsample_texture(frame_index);
		gpu_index_t		   input_index		  = ctx.get_lighting_texture_index(frame_index);

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout_compute(cmd, {.layout = render_globals_t::get_global_compute_bind_layout()});
		backend.cmd_bind_constants_compute(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const gpu_index_t bloom_render_pass_data_index = ctx.get_bloom_render_pass_data_index(frame_index);
		backend.cmd_bind_constants_compute(cmd, {.data = &bloom_render_pass_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});

		backend.cmd_bind_pipeline_compute(cmd, {.pipeline = ctx.get_bloom_downsample_shader()});

		for (u8 level = 0; level < WORLD_RENDER_BLOOM_LEVEL_COUNT; ++level)
		{
			const u32		width		  = std::max<u32>(1, size.x >> (level + 1));
			const u32		height		  = std::max<u32>(1, size.y >> (level + 1));
			const barrier_t write_barrier = {
				.from_states	= resource_state_non_ps_resource,
				.to_states		= resource_state_uav,
				.texture_t		= downsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = level,
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &write_barrier, .barrier_count = 1});

			const u32 constants[5] = {width, height, input_index, ctx.get_bloom_downsample_uav_index(frame_index, level), level};
			backend.cmd_bind_constants_compute(cmd, {.data = constants, .offset = constant_rp1, .count = 5, .param_index = 0});

			BEGIN_DEBUG_EVENT((&backend), cmd, "world_bloom_downsample");
			backend.cmd_dispatch(cmd, {.group_size_x = (width + 7) / 8, .group_size_y = (height + 7) / 8, .group_size_z = 1});
			END_DEBUG_EVENT((&backend), cmd);

			const barrier_t read_barrier = {
				.from_states	= resource_state_uav,
				.to_states		= resource_state_non_ps_resource,
				.texture_t		= downsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = level,
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &read_barrier, .barrier_count = 1});
			input_index = ctx.get_bloom_downsample_index(frame_index, level);
		}

		backend.cmd_bind_pipeline_compute(cmd, {.pipeline = ctx.get_bloom_upsample_shader()});
		input_index = ctx.get_bloom_downsample_index(frame_index, WORLD_RENDER_BLOOM_LEVEL_COUNT - 1);

		for (i32 level = WORLD_RENDER_BLOOM_LEVEL_COUNT - 1; level >= 0; --level)
		{
			const u32  width				= std::max<u32>(1, size.x >> level);
			const u32  height				= std::max<u32>(1, size.y >> level);
			const bool has_downsample_input = level > 0;

			const barrier_t write_barrier = {
				.from_states	= resource_state_non_ps_resource,
				.to_states		= resource_state_uav,
				.texture_t		= upsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = static_cast<u8>(level),
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &write_barrier, .barrier_count = 1});

			struct bloom_upsample_constants_t
			{
				u32			width;
				u32			height;
				gpu_index_t input;
				gpu_index_t downsample_input;
				gpu_index_t output;
				u32			has_downsample_input;
			};

			const bloom_upsample_constants_t constants = {
				.width				  = width,
				.height				  = height,
				.input				  = input_index,
				.downsample_input	  = has_downsample_input ? ctx.get_bloom_downsample_index(frame_index, static_cast<u8>(level - 1)) : NULL_GPU_INDEX,
				.output				  = ctx.get_bloom_upsample_uav_index(frame_index, static_cast<u8>(level)),
				.has_downsample_input = has_downsample_input ? 1u : 0u,
			};

			backend.cmd_bind_constants_compute(cmd, {.data = reinterpret_cast<const u32*>(&constants), .offset = constant_rp1, .count = 6, .param_index = 0});
			BEGIN_DEBUG_EVENT((&backend), cmd, "world_bloom_upsample");

			backend.cmd_dispatch(cmd, {.group_size_x = (width + 7) / 8, .group_size_y = (height + 7) / 8, .group_size_z = 1});
			END_DEBUG_EVENT((&backend), cmd);

			const barrier_t read_barrier = {
				.from_states	= resource_state_uav,
				.to_states		= resource_state_non_ps_resource,
				.texture_t		= upsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = static_cast<u8>(level),
				.mip_count		= 1,
			};

			backend.cmd_barrier(cmd, {.barriers = &read_barrier, .barrier_count = 1});
			input_index = ctx.get_bloom_upsample_index(frame_index, static_cast<u8>(level));
		}

		backend.close_command_buffer(cmd);
	}

	void world_rendering_t::render_post_process(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		const render_pass_data_post_process_gpu_t post_process_render_pass_data = {
			.params0 = vec4f_t(snapshot.post_process.bloom.strength, snapshot.post_process.exposure_ev, snapshot.post_process.saturation, snapshot.post_process.reinhard_white_point),
			.params1 = vec4f_t(snapshot.post_process.temperature, snapshot.post_process.tint, static_cast<f32>(snapshot.post_process.tonemap_mode), 0.0f),
		};
		SFG_MEMCPY(ctx.get_mapped_post_process_render_pass_data(frame_index), &post_process_render_pass_data, sizeof(render_pass_data_post_process_gpu_t));

		const gfx_handle_t cmd				= ctx.get_command_buffer_post(frame_index);
		const gfx_handle_t lighting_texture = ctx.get_lighting_texture(frame_index);
		const gfx_handle_t world_texture	= ctx.get_world_texture(frame_index);
		const bool		   bloom_active		= ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		barrier_t begin_barriers[3] = {};
		u16		  begin_count		= 0;

		if (bloom_active)
		{
			begin_barriers[begin_count++] = {
				.from_states = resource_state_non_ps_resource,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = lighting_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};

			begin_barriers[begin_count++] = {
				.from_states	= resource_state_non_ps_resource,
				.to_states		= resource_state_ps_resource,
				.texture_t		= ctx.get_bloom_upsample_texture(frame_index),
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = 0,
				.mip_count		= 1,
			};
		}

		begin_barriers[begin_count++] = {
			.from_states = resource_state_ps_resource,
			.to_states	 = resource_state_render_target,
			.texture_t	 = world_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = world_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 1,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "world_post_process");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		// post combiner blit.
		const render_resources_t& render_resources = render_resources_t::get();
		const gpu_index_t		  bloom_index	   = bloom_active ? ctx.get_bloom_upsample_index(frame_index, 0) : render_resources.get_texture_gpu_index(render_resources.get_black_texture(), 0);
		gpu_index_t				  rp_constants[3]  = {ctx.get_post_process_render_pass_data_index(frame_index), ctx.get_lighting_texture_index(frame_index), bloom_index};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 3, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_post_combiner_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		// debug draws
		const world_debug_draw_snapshot_t& debug_draw = snapshot.debug_draw;

		if (!debug_draw.line_indices.empty())
		{
			const gpu_index_t view_data_index = ctx.get_view_render_pass_data_index(frame_index);
			const gpu_index_t line_data_index = ctx.get_debug_line_data_index(frame_index);

			backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_constants(cmd, {.data = &line_data_index, .offset = constant_rp4, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_debug_line_shader()});
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = ctx.get_debug_line_vertex_buffer(frame_index), .slot = 0, .vertex_size = static_cast<u16>(sizeof(vertex_debug_line_t)), .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = ctx.get_debug_line_index_buffer(frame_index), .offset = 0, .index_size = static_cast<u8>(sizeof(primitive_index))});
			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = static_cast<u32>(debug_draw.line_indices.size()),
												   .instance_count			 = 1,
												   .start_index_location	 = 0,
												   .base_vertex_location	 = 0,
												   .start_instance_location	 = 0,
											   });
		}

		if (!debug_draw.text_indices.empty())
		{
			const gpu_index_t view_data_index	= ctx.get_view_render_pass_data_index(frame_index);
			const gpu_index_t text_data_index	= ctx.get_debug_text_data_index(frame_index);
			const gpu_index_t glyph_atlas_index = render_resources_t::get().get_texture_gpu_index(resource_manager_t::get().get_glyph_atlas().get_texture(), 0);

			backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_constants(cmd, {.data = &text_data_index, .offset = constant_rp4, .count = 1, .param_index = 0});
			backend.cmd_bind_constants(cmd, {.data = &glyph_atlas_index, .offset = constant_mat0, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_debug_text_shader()});
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = ctx.get_debug_text_vertex_buffer(frame_index), .slot = 0, .vertex_size = static_cast<u16>(sizeof(vertex_debug_text_t)), .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = ctx.get_debug_text_index_buffer(frame_index), .offset = 0, .index_size = static_cast<u8>(sizeof(primitive_index))});
			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = static_cast<u32>(debug_draw.text_indices.size()),
												   .instance_count			 = 1,
												   .start_index_location	 = 0,
												   .base_vertex_location	 = 0,
												   .start_instance_location	 = 0,
											   });
		}

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		barrier_t end_barriers[2] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = world_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};

		u16 end_count = 1;

		if (bloom_active)
		{
			end_barriers[end_count++] = {
				.from_states	= resource_state_ps_resource,
				.to_states		= resource_state_non_ps_resource,
				.texture_t		= ctx.get_bloom_upsample_texture(frame_index),
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = 0,
				.mip_count		= 1,
			};
		}

		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = end_count});
		backend.close_command_buffer(cmd);
	}
}
