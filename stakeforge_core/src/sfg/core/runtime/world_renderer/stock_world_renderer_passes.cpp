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

#include "stock_world_renderer_passes.hpp"
#include "world_renderer_view.hpp"
#include "stock_world_renderer_context.hpp"
#include "stock_world_renderer_snapshot.hpp"
#include "stock_world_renderer_util.hpp"
#include "stock_world_renderer_draw_common.hpp"
#include "core/runtime/render/render_resources.hpp"
#include "core/runtime/render/render_globals.hpp"
#include "core/gfx/backend/backend.hpp"
#include "core/gfx/common/barrier_description.hpp"
#include "core/gfx/common/commands.hpp"
#include "core/gfx/util/gfx_util.hpp"
#include "core/io/assert.hpp"
#include "core/memory/memory.hpp"
#include "core/runtime/engine/engine_threads.hpp"
#include "core/runtime/resources/resource_manager.hpp"
#include "core/runtime/resources/shader_types.hpp"
#include "core/runtime/ui/glyph_atlas.hpp"
#include "core/runtime/ui/ui_renderer.hpp"
#include <execution>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define STOCK_WORLD_RENDERER_PACKET_BATCH_SIZE				  128
#define STOCK_WORLD_RENDERER_REFLECTION_SPECULAR_SAMPLE_COUNT 1024
#define STOCK_WORLD_RENDERER_REFLECTION_SH_SAMPLE_COUNT		  4096

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

		void bind_material(gfx_backend& backend, gfx_handle_t cmd, u32& bound_index, u32 index, const stock_world_renderer_material_t& mat, u8 frame_index)
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

		void draw_skybox(gfx_backend& backend, gfx_handle_t cmd, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index)
		{
			if (snapshot.environment.material_index == UINT32_MAX)
				return;

			BEGIN_DEBUG_EVENT((&backend), cmd, "world_skybox");

			const stock_world_renderer_material_t& material		 = snapshot.materials[snapshot.environment.material_index];
			const render_resource_handle_t		   shader_handle = material.find_pso(0);
			SFG_ASSERT(!shader_handle.is_null());

			backend.cmd_bind_pipeline(cmd, {.pipeline = render_resources_t::get().get_shader_hw(shader_handle)});

			u32 bound_material = UINT32_MAX;

			bind_material(backend, cmd, bound_material, snapshot.environment.material_index, material, frame_index);
			backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 36, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

			END_DEBUG_EVENT((&backend), cmd);
		}
	}

	void stock_world_renderer_passes_t::draw_queue(
		gfx_handle_t cmd, const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, span_t<const stock_world_renderer_queue_item_t> draws, u32 required_pass_mask, u32 initial_variant_flags, u8 frame_index)
	{
		stock_world_renderer_draw_packet_t packets[STOCK_WORLD_RENDERER_PACKET_BATCH_SIZE] = {};
		stock_world_renderer_draw_state_t  state										   = {};
		u32								   draw_index									   = 0;

		while (draw_index < draws.size)
		{
			const u32 packet_count = stock_world_renderer_util_t::prep_draw_packets(ctx, snapshot, draws, required_pass_mask, initial_variant_flags, frame_index, {.data = packets, .size = std::size(packets)}, draw_index);
			draw_packets(cmd, {.data = packets, .size = packet_count}, {.data = snapshot.materials.data(), .size = snapshot.materials.size()}, frame_index, state);
		}
	}

	void stock_world_renderer_passes_t::draw_packets(gfx_handle_t cmd, span_t<const stock_world_renderer_draw_packet_t> packets, span_t<const stock_world_renderer_material_t> materials, u8 frame_index, stock_world_renderer_draw_state_t& state)
	{
		gfx_backend& backend = gfx_backend::get();

		// draw the prepared queue in order
		for (const stock_world_renderer_draw_packet_t& packet : packets)
		{
			bind_pipeline(backend, cmd, state.pipeline, packet.pipeline);

			if (packet.material_index != UINT32_MAX)
				bind_material(backend, cmd, state.material_index, packet.material_index, materials[packet.material_index], frame_index);

			const bool has_instance_buffer = packet.instance_buffer_index != NULL_GPU_INDEX;

			if (has_instance_buffer)
				backend.cmd_bind_constants(cmd, {.data = &packet.instance_buffer_index, .offset = constant_obj0, .count = 1, .param_index = 0});

			backend.cmd_bind_constants(cmd, {.data = packet.object_constants, .offset = has_instance_buffer ? constant_obj1 : constant_obj0, .count = packet.object_constant_count, .param_index = 0});

			if (!packet.vertex_buffer.is_null())
			{
				if (state.vertex_stride != packet.vertex_stride)
					state.vertex_buffer = {};

				bind_vertex(backend, cmd, state.vertex_buffer, packet.vertex_buffer, packet.vertex_stride);
				state.vertex_stride = packet.vertex_stride;
			}

			if (!packet.index_buffer.is_null())
			{
				if (state.index_stride != packet.index_stride)
					state.index_buffer = {};

				bind_index(backend, cmd, state.index_buffer, packet.index_buffer, packet.index_stride);
				state.index_stride = packet.index_stride;

				backend.cmd_draw_indexed_instanced(cmd,
												   {
													   .index_count_per_instance = packet.element_count,
													   .instance_count			 = packet.instance_count,
													   .start_index_location	 = packet.start_index,
													   .base_vertex_location	 = packet.start_vertex,
													   .start_instance_location	 = 0,
												   });
			}
			else
			{
				backend.cmd_draw_instanced(cmd,
										   {
											   .vertex_count_per_instance = packet.element_count,
											   .instance_count			  = packet.instance_count,
											   .start_vertex_location	  = packet.start_vertex,
											   .start_instance_location	  = 0,
										   });
			}
		}
	}

	void stock_world_renderer_passes_t::render_shadows(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& ss, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend&								 backend		= gfx_backend::get();
		const stock_world_renderer_shadow_context_t& shadow_context = ctx.get_shadow_context();
		const gfx_handle_t							 cmd			= shadow_context.get_command_buffer(frame_index);

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		inplace_vector_t<barrier_t, 32> barriers = {};

		for (const stock_world_renderer_shadow_view_t& view : prep_data.shadow_views)
		{
			if (view.view_index != 0)
				continue;

			barriers.push_back({.from_states = resource_state_ps_resource, .to_states = resource_state_depth_write, .texture = view.texture, .flags = barrier_flags::baf_is_texture});
		}

		backend.cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		BEGIN_DEBUG_EVENT((&backend), cmd, "world_shadow_pass");

		for (u32 view_index = 0; view_index < prep_data.shadow_views.size(); ++view_index)
		{
			const stock_world_renderer_shadow_view_t& view = prep_data.shadow_views[view_index];

			backend.cmd_begin_render_pass_depth_only(cmd, {.depth_stencil_attachment = {.texture = view.texture, .clear_depth = 1.0f, .depth_load_op = load_op::clear, .depth_store_op = store_op::store, .view_index = view.view_index}});
			backend.cmd_set_viewport(cmd, {.min_depth = 0.0f, .max_depth = 1.0f, .width = view.resolution.x, .height = view.resolution.y});
			backend.cmd_set_scissors(cmd, {.width = view.resolution.x, .height = view.resolution.y});

			const gpu_index_t rp_constants[3] = {shadow_context.get_view_data_index(frame_index, static_cast<u16>(view_index)), ctx.get_entity_buffer_index(frame_index), ctx.get_bone_buffer_index(frame_index)};

			backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 3, .param_index = 0});

			const stock_world_renderer_prep_view_t& prep_view = prep_data.views[view.cull_view_index];

			draw_queue(cmd, ctx, ss, prep_data.get_shadow_queue(prep_view), stock_world_renderer_pass_flags_shadow, shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering, frame_index);

			backend.cmd_end_render_pass(cmd, {});
		}

		END_DEBUG_EVENT((&backend), cmd);

		barriers.resize(0);

		for (const stock_world_renderer_shadow_view_t& view : prep_data.shadow_views)
		{
			if (view.view_index != 0)
				continue;

			barriers.push_back({.from_states = resource_state_depth_write, .to_states = resource_state_ps_resource, .texture = view.texture, .flags = barrier_flags::baf_is_texture});
		}

		backend.cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_probe(gfx_handle_t										 command_buffer,
													 const stock_world_renderer_context_t&				 ctx,
													 const stock_world_renderer_snapshot_t&				 snapshot,
													 const stock_world_renderer_prep_data_t&			 prep_data,
													 const stock_world_renderer_reflection_allocation_t& allocation,
													 const u16*											 cull_view_indices,
													 bool												 skybox_only,
													 u8													 frame_index,
													 gpu_index_t										 global_cbv_index,
													 gfx_handle_t										 global_layout)
	{
		gfx_backend&									 backend			= gfx_backend::get();
		const stock_world_renderer_reflection_context_t& reflection_context = ctx.get_reflection_context();
		const gpu_index_t								 fog_data_index		= ctx.get_fog_render_pass_data_index(frame_index);

		backend.reset_command_buffer(command_buffer);
		backend.cmd_bind_layout(command_buffer, {.layout = global_layout});
		backend.cmd_bind_constants(command_buffer, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const barrier_t radiance_write_barrier = {
			.from_states = resource_state_common,
			.to_states	 = resource_state_render_target,
			.texture	 = allocation.radiance_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(command_buffer, {.barriers = &radiance_write_barrier, .barrier_count = 1});

		BEGIN_DEBUG_EVENT((&backend), command_buffer, "world_reflection_probe");

		for (u8 face = 0; face < STOCK_WORLD_RENDERER_REFLECTION_FACE_COUNT; ++face)
		{
			const barrier_t depth_write_barrier = {
				.from_states = resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource,
				.to_states	 = resource_state_depth_write,
				.texture	 = allocation.depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(command_buffer, {.barriers = &depth_write_barrier, .barrier_count = 1});

			backend.cmd_begin_render_pass_depth_only(command_buffer,
													 {
														 .depth_stencil_attachment =
															 {
																 .texture		 = allocation.depth_texture,
																 .clear_stencil	 = 0,
																 .clear_depth	 = 0.0f,
																 .depth_load_op	 = load_op::clear,
																 .depth_store_op = store_op::store,
																 .view_index	 = 0,
															 },
													 });
			backend.cmd_set_viewport(command_buffer, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = allocation.resolution, .height = allocation.resolution});
			backend.cmd_set_scissors(command_buffer, {.x = 0, .y = 0, .width = allocation.resolution, .height = allocation.resolution});

			const gpu_index_t depth_constants[3] = {
				reflection_context.get_view_data_index(allocation, frame_index, face),
				ctx.get_entity_buffer_index(frame_index),
				ctx.get_bone_buffer_index(frame_index),
			};

			backend.cmd_bind_constants(command_buffer, {.data = depth_constants, .offset = constant_rp0, .count = 3, .param_index = 0});

			if (!skybox_only)
			{
				const stock_world_renderer_prep_view_t& prep_view = prep_data.views[cull_view_indices[face]];

				draw_queue(command_buffer, ctx, snapshot, prep_data.get_depth_queue(prep_view), stock_world_renderer_pass_flags_depth | stock_world_renderer_pass_flags_reflections, shader_variant_flags_z_prepass, frame_index);
			}

			backend.cmd_end_render_pass(command_buffer, {});

			const barrier_t depth_read_barrier = {
				.from_states = resource_state_depth_write,
				.to_states	 = resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource,
				.texture	 = allocation.depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(command_buffer, {.barriers = &depth_read_barrier, .barrier_count = 1});

			const render_pass_color_attachment_t color_attachment = {
				.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
				.texture	 = allocation.radiance_texture,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = face,
			};

			backend.cmd_begin_render_pass_depth_read_only(command_buffer,
														  {
															  .color_attachments = &color_attachment,
															  .depth_stencil_attachment =
																  {
																	  .texture			= allocation.depth_texture,
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

			const gpu_index_t forward_constants[4] = {
				reflection_context.get_view_data_index(allocation, frame_index, face),
				ctx.get_entity_buffer_index(frame_index),
				ctx.get_bone_buffer_index(frame_index),
				ctx.get_lighting_render_pass_data_index(frame_index),
			};

			backend.cmd_bind_constants(command_buffer, {.data = forward_constants, .offset = constant_rp0, .count = 4, .param_index = 0});
			backend.cmd_bind_constants(command_buffer, {.data = &fog_data_index, .offset = constant_rp5, .count = 1, .param_index = 0});

			draw_skybox(backend, command_buffer, snapshot, frame_index);

			if (!skybox_only)
			{
				const stock_world_renderer_prep_view_t& prep_view = prep_data.views[cull_view_indices[face]];

				draw_queue(command_buffer, ctx, snapshot, prep_data.get_opaque_queue(prep_view), stock_world_renderer_pass_flags_reflections, 0, frame_index);
				draw_queue(command_buffer, ctx, snapshot, prep_data.get_transparent_queue(prep_view), stock_world_renderer_pass_flags_reflections, 0, frame_index);
			}

			backend.cmd_end_render_pass(command_buffer, {});
		}

		END_DEBUG_EVENT((&backend), command_buffer);

		const barrier_t radiance_read_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_common,
			.texture	 = allocation.radiance_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(command_buffer, {.barriers = &radiance_read_barrier, .barrier_count = 1});
		backend.close_command_buffer(command_buffer);
	}

	void stock_world_renderer_passes_t::render_depth_prepass(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& ss, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
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
			.texture	 = depth_texture,
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

		const stock_world_renderer_prep_view_t& prep_view = prep_data.views[0];

		draw_queue(cmd, ctx, ss, prep_data.get_depth_queue(prep_view), stock_world_renderer_pass_flags_depth, shader_variant_flags_z_prepass, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t depth_read_barrier = {
			.from_states = resource_state_depth_write,
			.to_states	 = resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource,
			.texture	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &depth_read_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_gbuffer(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& ss, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
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
				.texture	 = gbuffer_textures[0],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_non_ps_resource | resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture	 = gbuffer_textures[1],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture	 = gbuffer_textures[2],
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_render_target,
				.texture	 = gbuffer_textures[3],
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

		const stock_world_renderer_prep_view_t& prep_view = prep_data.views[0];

		draw_queue(cmd, ctx, ss, prep_data.get_opaque_queue(prep_view), stock_world_renderer_pass_flags_gbuffer, shader_variant_flags_gbuffer, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barriers[4] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture	 = gbuffer_albedo,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_non_ps_resource | resource_state_ps_resource,
				.texture	 = gbuffer_normal,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture	 = gbuffer_orm,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture	 = gbuffer_emissive,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};

		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = 4});
		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_clustered_lighting(const stock_world_renderer_clustered_lighting_data_t& data, span_t<const stock_world_renderer_clustered_lighting_view_t> views)
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

		backend.cmd_bind_constants_compute(data.command_buffer, {.data = &data.lighting_data_index, .offset = constant_rp3, .count = 1, .param_index = 0});
		backend.cmd_bind_pipeline_compute(data.command_buffer, {.pipeline = data.shader});

		BEGIN_DEBUG_EVENT((&backend), data.command_buffer, "world_clustered_light_culling");

		for (size_t i = 0; i < views.size; ++i)
		{
			const stock_world_renderer_clustered_lighting_view_t& view = views.data[i];

			backend.cmd_bind_constants_compute(data.command_buffer, {.data = &view.view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_dispatch(data.command_buffer,
								 {
									 .group_size_x = (view.cluster_count_x + 3) / 4,
									 .group_size_y = (view.cluster_count_y + 3) / 4,
									 .group_size_z = view.cluster_count_z,
								 });
		}

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

	void stock_world_renderer_passes_t::render_prefilter_diffuse_sh(const stock_world_renderer_context_t& ctx, const stock_world_renderer_reflection_allocation_t& allocation, u8 frame_index, gpu_index_t global_cbv_index)
	{
		const stock_world_renderer_reflection_context_t& reflection_context = ctx.get_reflection_context();
		const gfx_handle_t								 command_buffer		= reflection_context.get_command_buffer_compute(frame_index);
		gfx_backend&									 backend			= gfx_backend::get();

		backend.reset_command_buffer(command_buffer);
		backend.cmd_bind_layout_compute(command_buffer, {.layout = render_globals_t::get_global_compute_bind_layout()});
		backend.cmd_bind_constants_compute(command_buffer, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const barrier_t begin_barriers[3] = {
			{
				.from_states = resource_state_common,
				.to_states	 = resource_state_non_ps_resource,
				.texture	 = allocation.radiance_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_common,
				.to_states	 = resource_state_uav,
				.texture	 = allocation.specular_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_common,
				.to_states	 = resource_state_uav,
				.resource_t	 = reflection_context.get_diffuse_sh_buffer(),
				.flags		 = barrier_flags::baf_is_resource,
			},
		};

		backend.cmd_barrier(command_buffer, {.barriers = begin_barriers, .barrier_count = 3});

		BEGIN_DEBUG_EVENT((&backend), command_buffer, "world_reflection_specular_prefilter");
		backend.cmd_bind_pipeline_compute(command_buffer, {.pipeline = reflection_context.get_specular_prefilter_shader()});

		for (u8 mip = 0; mip < allocation.specular_mip_count; ++mip)
		{
			const u32 mip_size	= std::max<u32>(1, static_cast<u32>(allocation.resolution) >> mip);
			const f32 roughness = allocation.specular_mip_count > 1 ? static_cast<f32>(mip) / static_cast<f32>(allocation.specular_mip_count - 1) : 0.0f;

			const struct
			{
				gpu_index_t source_texture_index	  = NULL_GPU_INDEX;
				gpu_index_t destination_texture_index = NULL_GPU_INDEX;
				u32			destination_size		  = 0;
				u32			sample_count			  = 0;
				f32			roughness				  = 0.0f;
				u32			source_size				  = 0;
			} constants = {
				.source_texture_index	   = allocation.radiance_texture_index,
				.destination_texture_index = allocation.specular_texture_uav_indices[mip],
				.destination_size		   = mip_size,
				.sample_count			   = STOCK_WORLD_RENDERER_REFLECTION_SPECULAR_SAMPLE_COUNT,
				.roughness				   = roughness,
				.source_size			   = allocation.resolution,
			};

			backend.cmd_bind_constants_compute(command_buffer, {.data = &constants, .offset = constant_rp0, .count = 6, .param_index = 0});
			backend.cmd_dispatch(command_buffer,
								 {
									 .group_size_x = (mip_size + 7) / 8,
									 .group_size_y = (mip_size + 7) / 8,
									 .group_size_z = STOCK_WORLD_RENDERER_REFLECTION_FACE_COUNT,
								 });
		}

		END_DEBUG_EVENT((&backend), command_buffer);

		BEGIN_DEBUG_EVENT((&backend), command_buffer, "world_reflection_diffuse_sh");
		backend.cmd_bind_pipeline_compute(command_buffer, {.pipeline = reflection_context.get_diffuse_sh_shader()});

		const gpu_index_t diffuse_sh_constants[4] = {
			allocation.radiance_texture_index,
			reflection_context.get_diffuse_sh_buffer_uav_index(),
			allocation.diffuse_sh_coefficient_offset,
			STOCK_WORLD_RENDERER_REFLECTION_SH_SAMPLE_COUNT,
		};

		backend.cmd_bind_constants_compute(command_buffer, {.data = diffuse_sh_constants, .offset = constant_rp0, .count = 4, .param_index = 0});
		backend.cmd_dispatch(command_buffer, {.group_size_x = 1, .group_size_y = 1, .group_size_z = 1});
		END_DEBUG_EVENT((&backend), command_buffer);

		const barrier_t end_barriers[3] = {
			{
				.from_states = resource_state_non_ps_resource,
				.to_states	 = resource_state_common,
				.texture	 = allocation.radiance_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_uav,
				.to_states	 = resource_state_common,
				.texture	 = allocation.specular_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_uav,
				.to_states	 = resource_state_common,
				.resource_t	 = reflection_context.get_diffuse_sh_buffer(),
				.flags		 = barrier_flags::baf_is_resource,
			},
		};

		backend.cmd_barrier(command_buffer, {.barriers = end_barriers, .barrier_count = 3});
		backend.close_command_buffer(command_buffer);
	}

	void stock_world_renderer_passes_t::render_ssao(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index)
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
			.texture	 = ao_half,
			.flags		 = barrier_flags::baf_is_texture,
		};
		begin_barriers[1] = {
			.from_states = resource_state_non_ps_resource,
			.to_states	 = resource_state_uav,
			.texture	 = ao_full,
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
			.texture	 = ao_half,
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
			.texture	 = ao_full,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &full_read_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_lighting(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
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
				.texture	 = lighting_texture,
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
				.texture	 = ctx.get_ao_texture(frame_index),
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

		const gpu_index_t fog_data_index = ctx.get_fog_render_pass_data_index(frame_index);
		backend.cmd_bind_constants(cmd, {.data = &fog_data_index, .offset = constant_rp5, .count = 1, .param_index = 0});

		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_lighting_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		if (ssao_active)
		{
			const barrier_t end_barrier = {
				.from_states = resource_state_ps_resource,
				.to_states	 = resource_state_non_ps_resource,
				.texture	 = ctx.get_ao_texture(frame_index),
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
		}

		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_forward(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend&	   backend		= gfx_backend::get();
		const gfx_handle_t cmd			= ctx.get_command_buffer_forward(frame_index);
		const bool		   bloom_active = ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;

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

		const gpu_index_t fog_data_index = ctx.get_fog_render_pass_data_index(frame_index);

		backend.cmd_bind_constants(cmd, {.data = &fog_data_index, .offset = constant_rp5, .count = 1, .param_index = 0});

		draw_skybox(backend, cmd, snapshot, frame_index);

		const stock_world_renderer_prep_view_t& prep_view = prep_data.views[0];

		draw_queue(cmd, ctx, snapshot, prep_data.get_transparent_queue(prep_view), stock_world_renderer_pass_flags_forward, 0, frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const span_t<const stock_world_renderer_queue_item_t> view_model_draws = prep_data.get_view_model_queue();

		if (view_model_draws.size != 0)
		{
			const gfx_handle_t view_model_depth				  = ctx.get_view_model_depth_texture(frame_index);
			const barrier_t	   view_model_depth_write_barrier = {
				.from_states = resource_state_depth_read,
				.to_states	 = resource_state_depth_write,
				.texture	 = view_model_depth,
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(cmd, {.barriers = &view_model_depth_write_barrier, .barrier_count = 1});

			BEGIN_DEBUG_EVENT((&backend), cmd, "world_view_model");
			backend.cmd_begin_render_pass_depth_only(cmd,
													 {
														 .depth_stencil_attachment =
															 {
																 .texture		 = view_model_depth,
																 .clear_stencil	 = 0,
																 .clear_depth	 = 0.0f,
																 .depth_load_op	 = load_op::clear,
																 .depth_store_op = store_op::store,
																 .view_index	 = 0,
															 },
													 });

			backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
			backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

			draw_queue(cmd, ctx, snapshot, prep_data.get_view_model_depth_queue(), stock_world_renderer_pass_flags_depth, shader_variant_flags_z_prepass, frame_index);

			backend.cmd_end_render_pass(cmd, {});

			const barrier_t view_model_depth_read_barrier = {
				.from_states = resource_state_depth_write,
				.to_states	 = resource_state_depth_read,
				.texture	 = view_model_depth,
				.flags		 = barrier_flags::baf_is_texture,
			};

			backend.cmd_barrier(cmd, {.barriers = &view_model_depth_read_barrier, .barrier_count = 1});

			backend.cmd_begin_render_pass_depth_read_only(cmd,
														  {
															  .color_attachments = &color_attachment,
															  .depth_stencil_attachment =
																  {
																	  .texture		  = view_model_depth,
																	  .clear_depth	  = 0.0f,
																	  .depth_load_op  = load_op::load,
																	  .depth_store_op = store_op::store,
																	  .view_index	  = 1,
																  },
															  .color_attachment_count = 1,
														  });
			backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
			backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

			draw_queue(cmd, ctx, snapshot, view_model_draws, 0, 0, frame_index);

			backend.cmd_end_render_pass(cmd, {});
			END_DEBUG_EVENT((&backend), cmd);
		}

		const ui::vg_draw_snapshot_t canvas_snapshot = snapshot.canvas.before_post_process.get_snapshot();

		if (canvas_snapshot.batch_count != 0)
		{
			backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
			ctx.get_canvas_before_renderer().render(cmd, &canvas_snapshot, frame_index, size);
			backend.cmd_end_render_pass(cmd, {});
		}

		const barrier_t end_barriers[3] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = bloom_active ? resource_state_non_ps_resource : resource_state_ps_resource,
				.texture	 = ctx.get_lighting_texture(frame_index),
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

	void stock_world_renderer_passes_t::render_bloom(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index)
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

		for (u8 level = 0; level < STOCK_WORLD_RENDERER_BLOOM_LEVEL_COUNT; ++level)
		{
			const u32		width		  = std::max<u32>(1, size.x >> (level + 1));
			const u32		height		  = std::max<u32>(1, size.y >> (level + 1));
			const barrier_t write_barrier = {
				.from_states	= resource_state_non_ps_resource,
				.to_states		= resource_state_uav,
				.texture		= downsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = level,
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &write_barrier, .barrier_count = 1});

			struct stock_world_renderer_bloom_downsample_constants_t
			{
				u32			width  = 0;
				u32			height = 0;
				gpu_index_t input  = NULL_GPU_INDEX;
				gpu_index_t output = NULL_GPU_INDEX;
				u32			level  = 0;
			};

			const stock_world_renderer_bloom_downsample_constants_t constants{
				.width	= width,
				.height = height,
				.input	= input_index,
				.output = ctx.get_bloom_downsample_uav_index(frame_index, level),
				.level	= level,
			};

			backend.cmd_bind_constants_compute(cmd, {.data = reinterpret_cast<const u32*>(&constants), .offset = constant_rp1, .count = 5, .param_index = 0});

			BEGIN_DEBUG_EVENT((&backend), cmd, "world_bloom_downsample");
			backend.cmd_dispatch(cmd, {.group_size_x = (width + 7) / 8, .group_size_y = (height + 7) / 8, .group_size_z = 1});
			END_DEBUG_EVENT((&backend), cmd);

			const barrier_t read_barrier = {
				.from_states	= resource_state_uav,
				.to_states		= resource_state_non_ps_resource,
				.texture		= downsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = level,
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &read_barrier, .barrier_count = 1});
			input_index = ctx.get_bloom_downsample_index(frame_index, level);
		}

		backend.cmd_bind_pipeline_compute(cmd, {.pipeline = ctx.get_bloom_upsample_shader()});
		input_index = ctx.get_bloom_downsample_index(frame_index, STOCK_WORLD_RENDERER_BLOOM_LEVEL_COUNT - 1);

		for (i32 level = STOCK_WORLD_RENDERER_BLOOM_LEVEL_COUNT - 1; level >= 0; --level)
		{
			const u32  width				= std::max<u32>(1, size.x >> level);
			const u32  height				= std::max<u32>(1, size.y >> level);
			const bool has_downsample_input = level > 0;

			const barrier_t write_barrier = {
				.from_states	= resource_state_non_ps_resource,
				.to_states		= resource_state_uav,
				.texture		= upsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = static_cast<u8>(level),
				.mip_count		= 1,
			};
			backend.cmd_barrier(cmd, {.barriers = &write_barrier, .barrier_count = 1});

			struct stock_world_renderer_bloom_upsample_constants_t
			{
				u32			width				 = 0;
				u32			height				 = 0;
				gpu_index_t input				 = NULL_GPU_INDEX;
				gpu_index_t downsample_input	 = NULL_GPU_INDEX;
				gpu_index_t output				 = NULL_GPU_INDEX;
				u32			has_downsample_input = 0;
			};

			const stock_world_renderer_bloom_upsample_constants_t constants = {
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
				.texture		= upsample_texture,
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = static_cast<u8>(level),
				.mip_count		= 1,
			};

			backend.cmd_barrier(cmd, {.barriers = &read_barrier, .barrier_count = 1});
			input_index = ctx.get_bloom_upsample_index(frame_index, static_cast<u8>(level));
		}

		backend.close_command_buffer(cmd);
	}

	void stock_world_renderer_passes_t::render_post_process(
		const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		gfx_backend& backend = gfx_backend::get();

		const vec2u16_t												   size							 = ctx.get_size();
		const stock_world_renderer_render_pass_data_post_process_gpu_t post_process_render_pass_data = {
			.params0 = vec4f_t(snapshot.post_process.bloom.strength, snapshot.post_process.exposure_ev, snapshot.post_process.saturation, snapshot.post_process.reinhard_white_point),
			.params1 = vec4f_t(snapshot.post_process.temperature, snapshot.post_process.tint, static_cast<f32>(snapshot.post_process.tonemap_mode), 0.0f),
			.params2 = vec4f_t(1.0f / static_cast<f32>(size.x), 1.0f / static_cast<f32>(size.y), snapshot.post_process.fxaa.fixed_threshold, snapshot.post_process.fxaa.relative_threshold),
			.params3 = vec4f_t(snapshot.post_process.fxaa.subpixel_blending, 0.0f, 0.0f, 0.0f),
		};

		SFG_MEMCPY(ctx.get_mapped_post_process_render_pass_data(frame_index), &post_process_render_pass_data, sizeof(stock_world_renderer_render_pass_data_post_process_gpu_t));

		const gfx_handle_t cmd				= ctx.get_command_buffer_post(frame_index);
		const gfx_handle_t lighting_texture = ctx.get_lighting_texture(frame_index);
		const gfx_handle_t world_texture	= ctx.get_world_texture(frame_index);
		const bool		   bloom_active		= ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;
		const bool		   fxaa_active		= snapshot.post_process.fxaa.enabled != 0;
		const gfx_handle_t tonemap_target	= fxaa_active ? ctx.get_fxaa_texture(frame_index) : world_texture;

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		BEGIN_DEBUG_EVENT((&backend), cmd, "world_post_process");

		if (bloom_active)
		{
			const barrier_t bloom_barriers[2] = {
				{
					.from_states = resource_state_non_ps_resource,
					.to_states	 = resource_state_ps_resource,
					.texture	 = lighting_texture,
					.flags		 = barrier_flags::baf_is_texture,
				},
				{
					.from_states	= resource_state_non_ps_resource,
					.to_states		= resource_state_ps_resource,
					.texture		= ctx.get_bloom_upsample_texture(frame_index),
					.flags			= barrier_flags::baf_is_texture,
					.base_mip_level = 0,
					.mip_count		= 1,
				},
			};

			backend.cmd_barrier(cmd, {.barriers = bloom_barriers, .barrier_count = 2});
		}

		const render_resources_t& render_resources = render_resources_t::get();
		const gpu_index_t		  bloom_index	   = bloom_active ? ctx.get_bloom_upsample_index(frame_index, 0) : render_resources.get_texture_gpu_index(render_resources.get_black_texture(), 0);

		const barrier_t tonemap_barrier = {
			.from_states = resource_state_ps_resource,
			.to_states	 = resource_state_render_target,
			.texture	 = tonemap_target,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &tonemap_barrier, .barrier_count = 1});

		const render_pass_color_attachment_t tonemap_attachment = {
			.texture	= tonemap_target,
			.load_op	= load_op::dont_care,
			.store_op	= store_op::store,
			.view_index = 1,
		};
		const gpu_index_t tonemap_constants[3] = {
			ctx.get_post_process_render_pass_data_index(frame_index),
			ctx.get_lighting_texture_index(frame_index),
			bloom_index,
		};

		// engine post combine.
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &tonemap_attachment, .color_attachment_count = 1});
		backend.cmd_bind_constants(cmd, {.data = tonemap_constants, .offset = constant_rp0, .count = 3, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_post_combiner_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
		backend.cmd_end_render_pass(cmd, {});

		if (fxaa_active)
		{
			const barrier_t fxaa_barriers[2] = {
				{
					.from_states = resource_state_render_target,
					.to_states	 = resource_state_ps_resource,
					.texture	 = tonemap_target,
					.flags		 = barrier_flags::baf_is_texture,
				},
				{
					.from_states = resource_state_ps_resource,
					.to_states	 = resource_state_render_target,
					.texture	 = world_texture,
					.flags		 = barrier_flags::baf_is_texture,
				},
			};

			backend.cmd_barrier(cmd, {.barriers = fxaa_barriers, .barrier_count = 2});

			const render_pass_color_attachment_t fxaa_attachment = {
				.texture	= world_texture,
				.load_op	= load_op::dont_care,
				.store_op	= store_op::store,
				.view_index = 1,
			};
			const gpu_index_t fxaa_constants[2] = {
				ctx.get_post_process_render_pass_data_index(frame_index),
				ctx.get_fxaa_texture_index(frame_index),
			};

			backend.cmd_begin_render_pass(cmd, {.color_attachments = &fxaa_attachment, .color_attachment_count = 1});
			backend.cmd_bind_constants(cmd, {.data = fxaa_constants, .offset = constant_rp0, .count = 2, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_fxaa_shader()});
			backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
			backend.cmd_end_render_pass(cmd, {});
		}

		const ui::vg_draw_snapshot_t					  canvas_snapshot  = snapshot.canvas.after_post_process.get_snapshot();
		const stock_world_renderer_debug_draw_snapshot_t& debug_draw	   = snapshot.debug_draw;
		const bool										  has_canvas_draws = canvas_snapshot.batch_count != 0;
		const bool										  has_debug_draws  = !debug_draw.textures.empty() || !debug_draw.line_indices.empty() || !debug_draw.text_indices.empty() || !debug_draw.triangle_indices.empty();
		const bool										  has_overlays	   = has_canvas_draws || has_debug_draws;

		if (has_overlays)
		{
			const render_pass_color_attachment_t overlay_attachment = {
				.texture	= world_texture,
				.load_op	= load_op::load,
				.store_op	= store_op::store,
				.view_index = 1,
			};

			backend.cmd_begin_render_pass(cmd, {.color_attachments = &overlay_attachment, .color_attachment_count = 1});
		}

		if (has_canvas_draws)
		{
			ctx.get_canvas_after_renderer().render(cmd, &canvas_snapshot, frame_index, size);

			backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
			backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});
		}

		if (!debug_draw.textures.empty())
		{
			const gpu_index_t view_data_index	   = ctx.get_view_render_pass_data_index(frame_index);
			const gpu_index_t texture_buffer_index = ctx.get_debug_texture_buffer_index(frame_index);

			backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_constants(cmd, {.data = &texture_buffer_index, .offset = constant_obj0, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_debug_texture_shader()});
			backend.cmd_draw_instanced(cmd,
									   {
										   .vertex_count_per_instance = 6,
										   .instance_count			  = static_cast<u32>(debug_draw.textures.size()),
										   .start_vertex_location	  = 0,
										   .start_instance_location	  = 0,
									   });
		}

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

		if (!debug_draw.triangle_indices.empty())
		{
			const gpu_index_t view_data_index = ctx.get_view_render_pass_data_index(frame_index);

			backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_debug_triangle_shader()});
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = ctx.get_debug_triangle_vertex_buffer(frame_index), .slot = 0, .vertex_size = static_cast<u16>(sizeof(vertex_debug_triangle_t)), .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = ctx.get_debug_triangle_index_buffer(frame_index), .offset = 0, .index_size = static_cast<u8>(sizeof(primitive_index))});
			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = static_cast<u32>(debug_draw.triangle_indices.size()),
												   .instance_count			 = 1,
												   .start_index_location	 = 0,
												   .base_vertex_location	 = 0,
												   .start_instance_location	 = 0,
											   });
		}

		if (has_overlays)
			backend.cmd_end_render_pass(cmd, {});

		END_DEBUG_EVENT((&backend), cmd);

		barrier_t end_barriers[2] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture	 = world_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};

		u16 end_count = 1;

		if (bloom_active)
		{
			end_barriers[end_count++] = {
				.from_states	= resource_state_ps_resource,
				.to_states		= resource_state_non_ps_resource,
				.texture		= ctx.get_bloom_upsample_texture(frame_index),
				.flags			= barrier_flags::baf_is_texture,
				.base_mip_level = 0,
				.mip_count		= 1,
			};
		}

		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = end_count});
		backend.close_command_buffer(cmd);
	}
}
