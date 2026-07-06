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
#include "world_render_context.hpp"
#include "world_render_snapshot.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/math/mat3x3.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/runtime/resources/shader_types.hpp>

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
	}
	void world_rendering_t::render_world(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		gpu_entity_t* entity_buffer = reinterpret_cast<gpu_entity_t*>(ctx.get_mapped_entity_buffer(frame_index));

		{
			SFG_ASSERT(snapshot.entities.size() <= WORLD_RENDER_ENTITY_BUFFER_CAPACITY);
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

		render_view_t main_camera_view_t = {};
		main_camera_view_t.calculate(snapshot.main_view, ctx.get_size(), interpolation_alpha);

		world_render_prep_data_t prep_data;
		prep_data.draw_culls.resize(snapshot.draws.size());
		const u8 main_view_index = 0;
		for (size_t i = 0; i < snapshot.draws.size(); ++i)
		{
			const world_draw_t& draw = snapshot.draws[i];
			SFG_ASSERT(draw.entity_index < snapshot.entities.size());

			const mat4x4_t& model = entity_buffer[draw.entity_index].model;
			const mat3x3_t	linear_model(model[0], model[1], model[2], model[4], model[5], model[6], model[8], model[9], model[10]);
			const vec3f_t	position = model.get_translation();

			if (frustum_t::test(main_camera_view_t.frustum, draw.aabb, linear_model, position) == frustum_result::outside)
				prep_data.draw_culls[i].cull_mask |= 1ull << main_view_index;
		}

		const render_pass_data_opaque_gpu_t opaque_render_pass_data = {.view_proj = main_camera_view_t.view_proj};
		SFG_MEMCPY(ctx.get_mapped_opaque_render_pass_data(frame_index), &opaque_render_pass_data, sizeof(render_pass_data_opaque_gpu_t));

		const render_pass_data_lighting_gpu_t lighting_render_pass_data = {
			.inv_view_proj = main_camera_view_t.inv_view_proj,
			.inv_view	   = main_camera_view_t.inv_view,
			.camera_pos	   = vec4f_t(main_camera_view_t.pos.x, main_camera_view_t.pos.y, main_camera_view_t.pos.z, 1.0f),
			.skybox_params = vec4f_t(snapshot.skybox.intensity, snapshot.skybox.exposure, 0.0f, 0.0f),
		};
		SFG_MEMCPY(ctx.get_mapped_lighting_render_pass_data(frame_index), &lighting_render_pass_data, sizeof(render_pass_data_lighting_gpu_t));

		struct render_graph_state_t
		{
			const world_render_context_t*	ctx				 = nullptr;
			const world_render_snapshot_t*	snapshot		 = nullptr;
			const world_render_prep_data_t* prep_data		 = nullptr;
			gfx_handle_t					global_layout	 = {};
			gpu_index_t						global_cbv_index = NULL_GPU_INDEX;
			u8								frame_index		 = 0;
		};

		static render_graph_state_t render_graph_state;
		static job_graph_t			render_graph;
		static bool					render_graph_initialized = false;
		if (!render_graph_initialized)
		{
			render_graph.emplace([]() {
				const world_render_context_t&	ctx		  = *render_graph_state.ctx;
				const world_render_snapshot_t&	snapshot  = *render_graph_state.snapshot;
				const world_render_prep_data_t& prep_data = *render_graph_state.prep_data;
				const u8						frame	  = render_graph_state.frame_index;

				render_access_scope_t scope;
				gfx_backend&		  backend = gfx_backend::get();
				const gfx_handle_t	  cmd	  = ctx.get_command_buffer_gfx0(frame);
				backend.reset_command_buffer(cmd);
				backend.cmd_bind_layout(cmd, {.layout = render_graph_state.global_layout});
				gpu_index_t global_constants[1] = {render_graph_state.global_cbv_index};
				backend.cmd_bind_constants(cmd, {.data = global_constants, .offset = constant_global0, .count = 1, .param_index = 0});
				render_depth_prepass(ctx, snapshot, prep_data, frame);
				render_gbuffer(ctx, snapshot, prep_data, frame);
				backend.close_command_buffer(cmd);
			});
			render_graph.emplace([]() {
				const world_render_context_t&	ctx		  = *render_graph_state.ctx;
				const world_render_snapshot_t&	snapshot  = *render_graph_state.snapshot;
				const world_render_prep_data_t& prep_data = *render_graph_state.prep_data;
				const u8						frame	  = render_graph_state.frame_index;

				render_access_scope_t scope;
				gfx_backend&		  backend = gfx_backend::get();
				const gfx_handle_t	  cmd	  = ctx.get_command_buffer_gfx1(frame);
				backend.reset_command_buffer(cmd);
				backend.cmd_bind_layout(cmd, {.layout = render_graph_state.global_layout});
				gpu_index_t global_constants[1] = {render_graph_state.global_cbv_index};
				backend.cmd_bind_constants(cmd, {.data = global_constants, .offset = constant_global0, .count = 1, .param_index = 0});
				render_lighting(ctx, snapshot, prep_data, frame);
				render_post_process(ctx, snapshot, prep_data, frame);
				backend.close_command_buffer(cmd);
			});
			render_graph_initialized = true;
		}

		SFG_ASSERT(render_graph_state.ctx == nullptr);
		render_graph_state.ctx				= &ctx;
		render_graph_state.snapshot			= &snapshot;
		render_graph_state.prep_data		= &prep_data;
		render_graph_state.global_layout	= global_layout;
		render_graph_state.global_cbv_index = global_cbv_index;
		render_graph_state.frame_index		= frame_index;

		job_system_t::get().get_executor().run(render_graph).wait();

		render_graph_state.ctx		 = nullptr;
		render_graph_state.snapshot	 = nullptr;
		render_graph_state.prep_data = nullptr;

		const gfx_handle_t cmd_gfx0	 = ctx.get_command_buffer_gfx0(frame_index);
		const gfx_handle_t cmd_gfx1	 = ctx.get_command_buffer_gfx1(frame_index);
		const gfx_handle_t queue_gfx = backend.get_queue_gfx();
		const gfx_handle_t semaphore = ctx.get_gfx0_done_semaphore(frame_index);
		const u64		   value	 = ctx.next_gfx0_done_semaphore_value(frame_index);

		backend.submit_commands(queue_gfx, &cmd_gfx0, 1);
		backend.queue_signal(queue_gfx, &semaphore, &value, 1);
		backend.queue_wait(queue_gfx, &semaphore, &value, 1);
		backend.submit_commands(queue_gfx, &cmd_gfx1, 1);
	}

	void world_rendering_t::render_depth_prepass(const world_render_context_t& ctx, const world_render_snapshot_t& ss, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t	cmd				 = ctx.get_command_buffer_gfx0(frame_index);
		const gfx_handle_t	depth_texture	 = ctx.get_depth_texture(frame_index);
		const gfx_handle_t	gbuffer_albedo	 = ctx.get_gbuffer_albedo_texture(frame_index);
		const gfx_handle_t	gbuffer_normal	 = ctx.get_gbuffer_normal_texture(frame_index);
		const gfx_handle_t	gbuffer_orm		 = ctx.get_gbuffer_orm_texture(frame_index);
		const gfx_handle_t	gbuffer_emissive = ctx.get_gbuffer_emissive_texture(frame_index);
		render_resources_t& rr				 = render_resources_t::get();

		barrier_t begin_barriers[5] = {};
		u16		  begin_count		= 0;

		u32 state = backend.get_texture_state(depth_texture);
		if (state != resource_state_depth_write)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_depth_write,
				.texture_t	 = depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(gbuffer_albedo);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_albedo,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(gbuffer_normal);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_normal,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(gbuffer_orm);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_orm,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(gbuffer_emissive);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = gbuffer_emissive,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
			backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

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

		gpu_index_t rp_constants[2] = {ctx.get_opaque_render_pass_data_index(frame_index), ctx.get_entity_buffer_index(frame_index)};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 2, .param_index = 0});

		gfx_handle_t bound_vertex	= {};
		gfx_handle_t bound_index	= {};
		gfx_handle_t bound_pipeline = {};

		for (const world_draw_t& draw : ss.draws)
		{
			continue;

			if ((draw.pass_mask & world_pass_flags_depth) == 0)
				continue;

			SFG_ASSERT(draw.material_index != UINT32_MAX);
			const world_render_material_t& mat = ss.materials[draw.material_index];

			const render_resource_handle_t shader_handle = mat.find_pso(shader_variant_flags_z_prepass);
			if (shader_handle.is_null())
				continue;

			const gfx_handle_t vtx		= rr.get_resource(draw.vertex_buffer);
			const gfx_handle_t idx		= rr.get_resource(draw.index_buffer);
			const gfx_handle_t pipeline = rr.get_shader_hw(shader_handle);
			SFG_ASSERT(!vtx.is_null() && !idx.is_null() && !pipeline.is_null());

			bind_vertex(backend, cmd, bound_vertex, vtx, draw.vertex_stride);
			bind_index(backend, cmd, bound_index, idx, draw.index_stride);
			bind_pipeline(backend, cmd, bound_pipeline, pipeline);

			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = draw.index_count,
												   .instance_count			 = 1,
												   .start_index_location	 = draw.start_index,
												   .base_vertex_location	 = draw.start_vertex,
												   .start_instance_location	 = 0,
											   });
		}

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);
	}

	void world_rendering_t::render_gbuffer(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				= ctx.get_command_buffer_gfx0(frame_index);
		const gfx_handle_t depth_texture	= ctx.get_depth_texture(frame_index);
		const gfx_handle_t gbuffer_albedo	= ctx.get_gbuffer_albedo_texture(frame_index);
		const gfx_handle_t gbuffer_normal	= ctx.get_gbuffer_normal_texture(frame_index);
		const gfx_handle_t gbuffer_orm		= ctx.get_gbuffer_orm_texture(frame_index);
		const gfx_handle_t gbuffer_emissive = ctx.get_gbuffer_emissive_texture(frame_index);

		const u32 depth_state = backend.get_texture_state(depth_texture);
		if (depth_state != resource_state_depth_read)
		{
			const barrier_t barrier = {
				.from_states = depth_state,
				.to_states	 = resource_state_depth_read,
				.texture_t	 = depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
			backend.cmd_barrier(cmd, {.barriers = &barrier, .barrier_count = 1});
		}

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

		gpu_index_t rp_constants[2] = {ctx.get_opaque_render_pass_data_index(frame_index), ctx.get_entity_buffer_index(frame_index)};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 2, .param_index = 0});

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barriers[5] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = gbuffer_albedo,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
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
			{
				.from_states = resource_state_depth_read,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};
		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = 5});
	}

	void world_rendering_t::render_lighting(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				= ctx.get_command_buffer_gfx1(frame_index);
		const gfx_handle_t lighting_texture = ctx.get_lighting_texture(frame_index);
		const gfx_handle_t ao_texture		= ctx.get_ao_texture(frame_index);

		barrier_t begin_barriers[2] = {};
		u16		  begin_count		= 0;

		u32 lighting_state = backend.get_texture_state(lighting_texture);
		if (lighting_state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = lighting_state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = lighting_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		u32 ao_state = backend.get_texture_state(ao_texture);
		if (ao_state != resource_state_ps_resource)
		{
			begin_barriers[begin_count++] = {
				.from_states = ao_state,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = ao_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
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

		const render_resources_t& render_resources = render_resources_t::get();
		gpu_index_t				  rp_constants[11] = {
			  ctx.get_lighting_render_pass_data_index(frame_index),
			  ctx.get_gbuffer_albedo_index(frame_index),
			  ctx.get_gbuffer_normal_index(frame_index),
			  ctx.get_gbuffer_orm_index(frame_index),
			  ctx.get_gbuffer_emissive_index(frame_index),
			  ctx.get_depth_texture_index(frame_index),
			  ctx.get_ao_texture_index(frame_index),
			  snapshot.skybox.radiance.is_null() ? NULL_GPU_INDEX : render_resources.get_texture_gpu_index(snapshot.skybox.radiance, 0),
			  snapshot.skybox.irradiance.is_null() ? NULL_GPU_INDEX : render_resources.get_texture_gpu_index(snapshot.skybox.irradiance, 0),
			  snapshot.skybox.prefilter.is_null() ? NULL_GPU_INDEX : render_resources.get_texture_gpu_index(snapshot.skybox.prefilter, 0),
			  snapshot.skybox.brdf_lut.is_null() ? NULL_GPU_INDEX : render_resources.get_texture_gpu_index(snapshot.skybox.brdf_lut, 0),
		  };
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 11, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_lighting_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);
	}

	void world_rendering_t::render_forward(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
	}

	void world_rendering_t::render_post_process(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index)
	{
		gfx_backend& backend = gfx_backend::get();

		const render_pass_data_post_process_gpu_t post_process_render_pass_data = {};
		SFG_MEMCPY(ctx.get_mapped_post_process_render_pass_data(frame_index), &post_process_render_pass_data, sizeof(render_pass_data_post_process_gpu_t));

		const gfx_handle_t cmd				= ctx.get_command_buffer_gfx1(frame_index);
		const gfx_handle_t lighting_texture = ctx.get_lighting_texture(frame_index);
		const gfx_handle_t world_texture	= ctx.get_world_texture(frame_index);

		barrier_t begin_barriers[2] = {};
		u16		  begin_count		= 0;

		u32 state = backend.get_texture_state(lighting_texture);
		if (state != resource_state_ps_resource)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = lighting_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(world_texture);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = world_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
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

		gpu_index_t rp_constants[2] = {ctx.get_post_process_render_pass_data_index(frame_index), ctx.get_lighting_texture_index(frame_index)};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 2, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_post_combiner_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture_t	 = world_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
	}
}
