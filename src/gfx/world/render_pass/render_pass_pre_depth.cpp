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

#include "render_pass_pre_depth.hpp"
#include "resources/vertex.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/renderable_collector.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_pre_depth::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_OPAQUE, 8);

		gfx_backend* backend = gfx_backend::get();

		create_textures(size);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "depth_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "depth_ubo"});
		}
	}

	void render_pass_pre_depth::uninit()
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

	void render_pass_pre_depth::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_OPAQUE);

		renderable_collector::populate_draw_stream(pm, renderables, _draw_stream, material_flags::material_flags_is_gbuffer, shader_variant_flags::variant_flag_z_prepass, frame_index);

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj = main_camera_view.view_proj_matrix,
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_pre_depth::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gfx_id	depth_texture	 = pfd.depth_texture;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		static_vector<barrier, 1> barriers;
		static_vector<barrier, 1> barriers_after;

		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_depth_write,
			.resource	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "depth_pre_pass");

		backend->cmd_begin_render_pass_depth_only(cmd_buffer,
												  {
													  .depth_stencil_attachment =
														  {
															  .texture		  = depth_texture,
															  .clear_stencil  = 0,
															  .clear_depth	  = 0.0f,
															  .depth_load_op  = load_op::clear,
															  .depth_store_op = store_op::store,
															  .view_index	  = 0,
														  },
												  });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const u32 rp_constants[3] = {gpu_index_rp_ubo, p.gpu_index_entities, p.gpu_index_bones};
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});
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

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_depth_write,
			.to_states	 = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.resource	 = depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_pre_depth::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_pre_depth::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_texture(pfd.depth_texture);
		}
	}

	void render_pass_pre_depth::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.depth_texture = backend->create_texture({
				.texture_format		  = render_target_definitions::get_format_depth_default_read(),
				.depth_stencil_format = render_target_definitions::get_format_depth_default(),
				.size				  = sz,
				.flags				  = texture_flags::tf_depth_texture | texture_flags::tf_typeless | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views				  = {{.type = view_type::depth_stencil}, {.type = view_type::depth_stencil, .read_only = 1}, {.type = view_type::sampled}},
				.clear_values		  = {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name			  = "prepass_depth",
			});

			pfd.gpu_index_depth_texture = backend->get_texture_gpu_index(pfd.depth_texture, 2);
		}
	}
}
