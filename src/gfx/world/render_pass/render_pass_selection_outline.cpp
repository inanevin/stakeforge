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

#include "render_pass_selection_outline.hpp"
#include "resources/vertex.hpp"
#include "app/engine_resources.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/world/view.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/common/render_target_definitions.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_selection_outline::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_OPAQUE, 8);

		gfx_backend* backend = gfx_backend::get();

		create_textures(size);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "sel_outline_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "object_outline_ubo"});
		}
	}

	void render_pass_selection_outline::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
		}

		destroy_textures();
	}

	void render_pass_selection_outline::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, 1);

		if (_selected_entity_id != NULL_WORLD_ID)
		{
			const shader_direct& write_shader = engine_resources::get().get_shader_direct(engine_resource_ident::shader_object_outline_write);
			renderable_collector::populate_draw_stream_outline_filtered(pm, renderables, _draw_stream, 0, frame_index, write_shader, _selected_entity_id);
		}

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj = main_camera_view.view_proj_matrix,
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_selection_outline::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend					= gfx_backend::get();
		per_frame_data& pfd						= _pfd[p.frame_index];
		const gfx_id	cmd_buffer				= pfd.cmd_buffer;
		const gfx_id	render_target			= pfd.render_target;
		const gpu_index gpu_index_render_target = pfd.gpu_index_render_target;
		const gpu_index gpu_index_rp_ubo		= pfd.ubo.get_index();

		render_pass_color_attachment att_render_target = {};
		att_render_target.clear_color				   = vector4(0, 0, 0, 0.0f);
		att_render_target.load_op					   = load_op::clear;
		att_render_target.store_op					   = store_op::store;
		att_render_target.texture					   = render_target;

		static_vector<barrier, 2> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_render_target,
			.resource	 = render_target,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });
		barriers.resize(0);

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "object_outline_pass_write");
		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = &att_render_target,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		{
			const u32 rp_constants[3] = {gpu_index_rp_ubo, p.gpu_index_entities, p.gpu_index_bones};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});
		}

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(p.size.x), .height = static_cast<u16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<u16>(p.size.x),
									  .height	 = static_cast<u16>(p.size.y),

								  });

		_draw_stream.build();
		_draw_stream.draw(cmd_buffer);

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = render_target,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_selection_outline::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_selection_outline::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
			pfd.render_target = NULL_GFX_ID;
		}
	}

	void render_pass_selection_outline::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.render_target	= backend->create_texture({
				  .texture_format = render_target_definitions::get_format_selection(),
				  .size			  = sz,
				  .flags		  = texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				  .views		  = {{.type = view_type::render_target}, {.type = view_type::sampled}},
				  .clear_values	  = {0, 0, 0, 0},
				  .debug_name	  = "object_outline_rt",
			  });

			pfd.gpu_index_render_target = backend->get_texture_gpu_index(pfd.render_target, 1);
		}
	}
}
