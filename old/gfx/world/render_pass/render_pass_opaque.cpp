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

#include "render_pass_opaque.hpp"
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
	void render_pass_opaque::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_OPAQUE, 8);

		gfx_backend* backend = gfx_backend::get();

		create_textures(size);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "opaque_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "opaque_ubo"});
		}
	}

	void render_pass_opaque::uninit()
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

	void render_pass_opaque::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_OPAQUE);

		renderable_collector::populate_draw_stream(pm, renderables, _draw_stream, material_flags::material_flags_is_gbuffer, 0, frame_index);

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj = main_camera_view.view_proj_matrix,
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_opaque::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id*	textures		 = pfd.color_textures.data();
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		render_pass_color_attachment* attachments = _alloc.allocate<render_pass_color_attachment>(GBUFFER_COLOR_TEXTURES);

		static_vector<barrier, 8> barriers;
		static_vector<barrier, 8> barriers_after;

		for (u8 i = 0; i < GBUFFER_COLOR_TEXTURES; i++)
		{
			const gfx_id txt = textures[i];

			render_pass_color_attachment& att = attachments[i];
			att.clear_color					  = vector4(0, 0, 0, 1.0f);
			att.load_op						  = load_op::clear;
			att.store_op					  = store_op::store;
			att.texture						  = txt;
			att.view_index					  = 0;

			// normals for compute
			u32 extra_state = i == 1 ? resource_state_non_ps_resource : 0;

			barriers.push_back({
				.from_states = resource_state::resource_state_ps_resource | extra_state,
				.to_states	 = resource_state::resource_state_render_target,
				.resource	 = txt,
				.flags		 = barrier_flags::baf_is_texture,
			});

			barriers_after.push_back({
				.from_states = resource_state::resource_state_render_target,
				.to_states	 = resource_state::resource_state_ps_resource | extra_state,
				.resource	 = txt,
				.flags		 = barrier_flags::baf_is_texture,
			});
		}

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "opaque_pass");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = attachments,
														   .depth_stencil_attachment =
															   {
																   .texture		   = p.depth_texture,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = GBUFFER_COLOR_TEXTURES,
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
		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers_after.data(),
								 .barrier_count = static_cast<u16>(barriers_after.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_opaque::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_opaque::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			for (u32 i = 0; i < GBUFFER_COLOR_TEXTURES; i++)
				backend->destroy_texture(pfd.color_textures[i]);
			pfd.color_textures.clear();
		}
	}

	void render_pass_opaque::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		static_vector<const char*, GBUFFER_COLOR_TEXTURES> names;
		names.push_back("opaque_rt_albedo");
		names.push_back("opaque_rt_normal");
		names.push_back("opaque_rt_orm");
		names.push_back("opaque_rt_emissive");

		static_vector<format, GBUFFER_COLOR_TEXTURES> formats;

		formats.push_back(render_target_definitions::get_format_gbuffer_albedo());
		formats.push_back(render_target_definitions::get_format_gbuffer_normal());
		formats.push_back(render_target_definitions::get_format_gbuffer_orm());
		formats.push_back(render_target_definitions::get_format_gbuffer_emissive());
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.color_textures.clear();
			pfd.gpu_index_color_textures.clear();

			for (u32 j = 0; j < GBUFFER_COLOR_TEXTURES; j++)
			{
				pfd.color_textures.push_back(backend->create_texture({
					.texture_format = formats[j],
					.size			= sz,
					.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
					.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
					.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
					.debug_name		= names[j],
				}));
				pfd.gpu_index_color_textures.push_back(backend->get_texture_gpu_index(pfd.color_textures.back(), 1));
			}
		}
	}
}
