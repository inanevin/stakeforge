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

#include "world/editor_world_renderer.hpp"
#include "ui/editor_global_toolbar.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/bitmask.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_draw.hpp>
#include <sfg/runtime/render/world_render_material.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/shader_types.hpp>

#include <algorithm>

namespace sfg
{
	void editor_world_renderer_t::init(vec2u16_t size, span_t<world_render_snapshot_t> snapshots)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		_world_render_context.init(size);
		for (size_t i = 0; i < snapshots.size; ++i)
			snapshots.data[i].user_data = new editor_world_snapshot_data_t();

		gfx_backend&	backend				= gfx_backend::get();
		resource_desc_t composite_data_desc = {};
		composite_data_desc.size			= static_cast<u32>(sizeof(composite_data_t));
		composite_data_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		composite_data_desc.set_name("editor_world_composite_data");

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i].cmd_gfx		   = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "editor_world",
			});
			_pfd[i].composite_data = backend.create_resource(composite_data_desc);
			backend.map_resource(_pfd[i].composite_data, _pfd[i].mapped_composite_data);
			_pfd[i].composite_data_index = backend.get_resource_gpu_index(_pfd[i].composite_data);
		}

		const shader_internals_t* shader = resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/editor_world_render_texture.hlsl"_hs);
		_shader							 = render_resources_t::get().get_shader_hw(shader->psos[0]);

		create_texture(size);
	}

	void editor_world_renderer_t::uninit(span_t<world_render_snapshot_t> snapshots)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (size_t i = 0; i < snapshots.size; ++i)
		{
			delete static_cast<editor_world_snapshot_data_t*>(snapshots.data[i].user_data);
			snapshots.data[i].user_data = nullptr;
		}

		destroy_texture();
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			backend.destroy_resource(_pfd[i].composite_data);
			backend.destroy_command_buffer(_pfd[i].cmd_gfx);
			_pfd[i].cmd_gfx				  = {};
			_pfd[i].composite_data		  = {};
			_pfd[i].mapped_composite_data = nullptr;
			_pfd[i].composite_data_index  = NULL_GPU_INDEX;
		}
		_shader = {};
		_world_render_context.uninit();
	}

	void editor_world_renderer_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		_world_render_context.resize(size);
		destroy_texture();
		create_texture(size);
	}

	void editor_world_renderer_t::render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		world_rendering_t::render_world(_world_render_context, snapshot, interpolation_alpha, frame_index, global_cbv_index, global_layout);

		const editor_world_snapshot_data_t& snapshot_data = *static_cast<const editor_world_snapshot_data_t*>(snapshot.user_data);
		const bool							has_selection = !snapshot_data.selected_entities.empty();

		const composite_data_t composite_data = {
			.params			 = vec4f_t(static_cast<f32>(_size.x), static_cast<f32>(_size.y), 2.0f, has_selection ? 1.0f : 0.0f),
			.selection_color = editor_theme_t::get().color_accent2,
		};
		SFG_MEMCPY(_pfd[frame_index].mapped_composite_data, &composite_data, sizeof(composite_data_t));

		gpu_index_t source_texture_idx = NULL_GPU_INDEX;
		switch (editor_global_toolbar_t::get().get_world_view())
		{
		case editor_main_toolbar_world_view_e::gbuffer_albedo:
			source_texture_idx = _world_render_context.get_gbuffer_albedo_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_orm:
			source_texture_idx = _world_render_context.get_gbuffer_orm_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_normal:
			source_texture_idx = _world_render_context.get_gbuffer_normal_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::gbuffer_emissive:
			source_texture_idx = _world_render_context.get_gbuffer_emissive_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::lighting:
			source_texture_idx = _world_render_context.get_lighting_texture_index(frame_index);
			break;
		case editor_main_toolbar_world_view_e::post_process:
		case editor_main_toolbar_world_view_e::final:
		default:
			source_texture_idx = _world_render_context.get_world_texture_index(frame_index);
			break;
		}

		gfx_backend&		backend				  = gfx_backend::get();
		render_resources_t& render_resources	  = render_resources_t::get();
		const gfx_handle_t	cmd					  = _pfd[frame_index].cmd_gfx;
		const gfx_handle_t	editor_texture		  = _pfd[frame_index].world_texture;
		const gfx_handle_t	selection_texture	  = _pfd[frame_index].selection_texture;
		const gpu_index_t	selection_texture_idx = _pfd[frame_index].selection_texture_index;
		const gpu_index_t	composite_data_idx	  = _pfd[frame_index].composite_data_index;

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		barrier_t begin_barriers[2] = {};
		u16		  begin_count		= 0;

		u32 state = backend.get_texture_state(selection_texture);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = selection_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
			backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		const render_pass_color_attachment_t selection_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 0.0f),
			.texture	 = selection_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};
		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_selection_mask");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &selection_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = _size.x, .height = _size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = _size.x, .height = _size.y});

		gpu_index_t rp_constants[2] = {_world_render_context.get_opaque_render_pass_data_index(frame_index), _world_render_context.get_entity_buffer_index(frame_index)};
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = 2, .param_index = 0});

		gfx_handle_t bound_vertex	= {};
		gfx_handle_t bound_index	= {};
		gfx_handle_t bound_pipeline = {};
		u32			 bound_material = UINT32_MAX;

		for (const world_draw_t& draw : snapshot.draws)
		{
			SFG_ASSERT(draw.entity_index < snapshot.entities.size());
			const world_render_entity_t& entity = snapshot.entities[draw.entity_index];
			if (std::find(snapshot_data.selected_entities.begin(), snapshot_data.selected_entities.end(), entity.entity_id) == snapshot_data.selected_entities.end())
				continue;

			SFG_ASSERT(draw.material_index < snapshot.materials.size());
			const world_render_material_t& mat = snapshot.materials[draw.material_index];

			bitmask_t<u32> variant_flags = shader_variant_flags_selection_outline;
			variant_flags.set(shader_variant_flags_alpha_cutoff, mat.use_alpha_cutoff != 0);
			variant_flags.set(shader_variant_flags_double_sided, mat.double_sided != 0);
			variant_flags.set(shader_variant_flags_skinned, draw.skinning_index != UINT32_MAX);

			render_resource_handle_t shader_handle = mat.find_pso(variant_flags);
			if (shader_handle.is_null() && mat.use_alpha_cutoff != 0)
			{
				bitmask_t<u32> fallback_flags = shader_variant_flags_selection_outline;
				fallback_flags.set(shader_variant_flags_alpha_cutoff, true);
				fallback_flags.set(shader_variant_flags_skinned, draw.skinning_index != UINT32_MAX);
				shader_handle = mat.find_pso(fallback_flags);
			}
			if (shader_handle.is_null())
				shader_handle = mat.find_pso(shader_variant_flags_selection_outline);
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

		const barrier_t selection_end_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture_t	 = selection_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &selection_end_barrier, .barrier_count = 1});

		begin_count = 0;

		state = backend.get_texture_state(editor_texture);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = editor_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
			backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = editor_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};
		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_render_texture");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = _size.x, .height = _size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = _size.x, .height = _size.y});
		backend.cmd_bind_constants(cmd, {.data = &composite_data_idx, .offset = constant_rp0, .count = 1, .param_index = 0});
		const gpu_index_t obj_constants[2] = {source_texture_idx, selection_texture_idx};
		backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = _shader});
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

	void editor_world_renderer_t::create_texture(vec2u16_t size)
	{
		texture_desc_t desc = {};
		desc.texture_format = format_e::r8g8b8a8_srgb;
		desc.size			= size;
		desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		desc.view_count		= 2;
		desc.views[0]		= {.type = view_type::render_target};
		desc.views[1]		= {.type = view_type::sampled};
		desc.set_name("editor_world_texture");

		texture_desc_t selection_desc  = desc;
		selection_desc.texture_format  = format_e::r8g8b8a8_unorm;
		selection_desc.clear_values[0] = 0.0f;
		selection_desc.clear_values[1] = 0.0f;
		selection_desc.clear_values[2] = 0.0f;
		selection_desc.clear_values[3] = 0.0f;
		selection_desc.set_name("editor_world_selection");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].world_texture.is_null());
			SFG_ASSERT(_pfd[i].selection_texture.is_null());

			_pfd[i].world_texture			= backend.create_texture(desc);
			_pfd[i].selection_texture		= backend.create_texture(selection_desc);
			_pfd[i].world_texture_index		= backend.get_texture_gpu_index(_pfd[i].world_texture, 1);
			_pfd[i].selection_texture_index = backend.get_texture_gpu_index(_pfd[i].selection_texture, 1);
		}
		_size = size;
	}

	void editor_world_renderer_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].world_texture.is_null());
			SFG_ASSERT(!_pfd[i].selection_texture.is_null());

			backend.destroy_texture(_pfd[i].world_texture);
			backend.destroy_texture(_pfd[i].selection_texture);
			_pfd[i].world_texture			= {};
			_pfd[i].selection_texture		= {};
			_pfd[i].world_texture_index		= NULL_GPU_INDEX;
			_pfd[i].selection_texture_index = NULL_GPU_INDEX;
		}
		_size = vec2u16_t::zero;
	}
}
