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
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>

namespace sfg
{
	void editor_world_renderer_t::init(vec2u16_t size, span_t<world_render_snapshot_t> snapshots)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		_world_render_context.init(size);
		for (size_t i = 0; i < snapshots.size; ++i)
			snapshots.data[i].user_data = new editor_world_snapshot_data_t();

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i].cmd_gfx = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "editor_world",
			});
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
			backend.destroy_command_buffer(_pfd[i].cmd_gfx);
			_pfd[i].cmd_gfx = {};
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

		gfx_backend&	   backend		  = gfx_backend::get();
		const gfx_handle_t cmd			  = _pfd[frame_index].cmd_gfx;
		const gfx_handle_t editor_texture = _pfd[frame_index].world_texture;

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});

		barrier_t begin_barriers[2] = {};
		u16		  begin_count		= 0;

		u32 state = backend.get_texture_state(editor_texture);
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
		backend.cmd_bind_constants(cmd, {.data = &source_texture_idx, .offset = constant_obj0, .count = 1, .param_index = 0});
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

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].world_texture.is_null());

			_pfd[i].world_texture		= backend.create_texture(desc);
			_pfd[i].world_texture_index = backend.get_texture_gpu_index(_pfd[i].world_texture, 1);
		}
		_size = size;
	}

	void editor_world_renderer_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].world_texture.is_null());

			backend.destroy_texture(_pfd[i].world_texture);
			_pfd[i].world_texture		= {};
			_pfd[i].world_texture_index = NULL_GPU_INDEX;
		}
		_size = vec2u16_t::zero;
	}
}
