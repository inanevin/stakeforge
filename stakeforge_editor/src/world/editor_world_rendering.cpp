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

#include "world/editor_world_rendering.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_render_context.hpp"
#include "ui/editor_global_toolbar.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/bitmask.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_draw.hpp>
#include <sfg/runtime/render/world_render_material.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/resources/shader_types.hpp>

namespace sfg
{
	void editor_world_rendering_t::render_outlines(const editor_world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		const editor_world_snapshot_data_t& snapshot_data = *static_cast<const editor_world_snapshot_data_t*>(snapshot.user_data);

		gfx_backend& backend = gfx_backend::get();

		render_resources_t& render_resources = render_resources_t::get();

		const gfx_handle_t cmd = ctx.get_command_buffer(frame_index);

		const gfx_handle_t selection_texture = ctx.get_selection_texture(frame_index);

		const vec2u16_t size = ctx.get_size();

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		const u32 state = backend.get_texture_state(selection_texture);
		if (state != resource_state_render_target)
		{
			const barrier_t begin_barrier = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = selection_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
			backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});
		}

		const render_pass_color_attachment_t selection_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 0.0f),
			.texture	 = selection_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_selection_mask");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &selection_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		gpu_index_t rp_constants[2] = {
			ctx.get_world_render_context().get_opaque_render_pass_data_index(frame_index),
			ctx.get_world_render_context().get_entity_buffer_index(frame_index),
		};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 2, .param_index = 0});

		gfx_handle_t bound_vertex	= {};
		gfx_handle_t bound_index	= {};
		gfx_handle_t bound_pipeline = {};
		u32			 bound_material = UINT32_MAX;

		for (const world_draw_t& draw : snapshot.draws)
		{
			const world_render_entity_t& entity = snapshot.entities[draw.entity_index];
			if (std::find(snapshot_data.selected_entities.begin(), snapshot_data.selected_entities.end(), entity.entity_id) == snapshot_data.selected_entities.end())
				continue;

			const world_render_material_t& mat = snapshot.materials[draw.material_index];

			bitmask_t<u32> variant_flags = shader_variant_flags_selection_outline;
			variant_flags.set(shader_variant_flags_alpha_cutoff, mat.use_alpha_cutoff != 0);
			variant_flags.set(shader_variant_flags_double_sided, mat.double_sided != 0);
			variant_flags.set(shader_variant_flags_skinned, draw.skinning_index != UINT32_MAX);

			const render_resource_handle_t shader_handle = mat.find_pso(variant_flags);
			if (shader_handle.is_null())
				continue;

			const gfx_handle_t vtx = render_resources.get_resource(draw.vertex_buffer);
			const gfx_handle_t idx = render_resources.get_resource(draw.index_buffer);
			SFG_ASSERT(!vtx.is_null() && !idx.is_null());

			if (vtx != bound_vertex)
			{
				bound_vertex = vtx;
				backend.cmd_bind_vertex_buffers(cmd, {.buffer = bound_vertex, .slot = 0, .vertex_size = draw.vertex_stride, .offset = 0});
			}

			if (idx != bound_index)
			{
				bound_index = idx;
				backend.cmd_bind_index_buffers(cmd, {.buffer = bound_index, .offset = 0, .index_size = draw.index_stride});
			}

			const gfx_handle_t pipeline = render_resources.get_shader_hw(shader_handle);
			if (pipeline != bound_pipeline)
			{
				bound_pipeline = pipeline;
				backend.cmd_bind_pipeline(cmd, {.pipeline = bound_pipeline});
			}

			if (bound_material != draw.material_index)
			{
				bound_material												 = draw.material_index;
				gpu_index_t mat_constants[1 + SFG_MATERIAL_MAX_TEXTURES * 2] = {};
				u8			mat_constant_count								 = 0;
				mat_constants[mat_constant_count++]							 = render_resources.get_resource_gpu_index(mat.material_buffer);
				for (u32 i = 0; i < mat.texture_count; ++i)
					mat_constants[mat_constant_count++] = render_resources.get_texture_gpu_index(mat.material_textures[i], 0);
				for (u32 i = 0; i < mat.texture_count; ++i)
					mat_constants[mat_constant_count++] = render_resources.get_sampler_gpu_index(mat.material_samplers[i]);
				backend.cmd_bind_constants(cmd, {.data = mat_constants, .offset = constant_mat0, .count = mat_constant_count, .param_index = 0});
			}

			const u32 obj_constants[2] = {draw.entity_index, draw.skinning_index == UINT32_MAX ? 0 : draw.skinning_index};
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

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture_t	 = selection_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
	}

	void editor_world_rendering_t::blit_world_texture(const editor_world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index)
	{
		const editor_world_snapshot_data_t& snapshot_data = *static_cast<const editor_world_snapshot_data_t*>(snapshot.user_data);

		const vec2u16_t size = ctx.get_size();

		const editor_world_composite_data_t composite_data = {
			.params			 = vec4f_t(static_cast<f32>(size.x), static_cast<f32>(size.y), 2.0f, snapshot_data.selected_entities.empty() ? 0.0f : 1.0f),
			.selection_color = editor_theme_t::get().color_accent2,
		};
		SFG_MEMCPY(ctx.get_mapped_composite_data(frame_index), &composite_data, sizeof(editor_world_composite_data_t));

		const world_render_context_t& world_ctx = ctx.get_world_render_context();

		gpu_index_t source_texture_index = NULL_GPU_INDEX;
		switch (editor_global_toolbar_t::get().get_world_view())
		{
		case editor_main_toolbar_world_view_e::gbuffer_albedo:
			source_texture_index = world_ctx.get_gbuffer_albedo_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_orm:
			source_texture_index = world_ctx.get_gbuffer_orm_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_normal:
			source_texture_index = world_ctx.get_gbuffer_normal_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_emissive:
			source_texture_index = world_ctx.get_gbuffer_emissive_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::lighting:
			source_texture_index = world_ctx.get_lighting_texture_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::post_process:
		case editor_main_toolbar_world_view_e::final:
		default:
			source_texture_index = world_ctx.get_world_texture_index(frame_index);
			break;
		}

		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd = ctx.get_command_buffer(frame_index);

		const gfx_handle_t editor_texture = ctx.get_world_texture(frame_index);

		const gpu_index_t selection_texture_index = ctx.get_selection_texture_index(frame_index);

		const gpu_index_t composite_data_index = ctx.get_composite_data_index(frame_index);

		const u32 state = backend.get_texture_state(editor_texture);
		if (state != resource_state_render_target)
		{
			const barrier_t begin_barrier = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = editor_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
			backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});
		}

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = editor_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_render_texture");

		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});
		backend.cmd_bind_constants(cmd, {.data = &composite_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
		const gpu_index_t obj_constants[2] = {source_texture_index, selection_texture_index};
		backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
		backend.cmd_end_render_pass(cmd, {});

		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture_t	 = editor_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);

		const gfx_handle_t queue_gfx = backend.get_queue_gfx();
		backend.submit_commands(queue_gfx, &cmd, 1);
	}
}
