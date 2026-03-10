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

#include "render_pass_object_id.hpp"
#include "resources/vertex.hpp"

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
	void render_pass_object_id::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_ALL_OBJECTS, 8);

		gfx_backend* backend = gfx_backend::get();

		create_textures(size);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "object_id_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "object_id_ubo"});
		}
	}

	void render_pass_object_id::uninit()
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

	void render_pass_object_id::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_ALL_OBJECTS);

		renderable_collector::populate_draw_stream(pm, renderables, _draw_stream, 0, shader_variant_flags::variant_flag_id_write, frame_index);

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj = main_camera_view.view_proj_matrix,
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_object_id::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gfx_id	render_target	 = pfd.render_target;
		const gfx_id	readback		 = pfd.readback_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		render_pass_color_attachment att = {};
		att.clear_color					 = vector4(UINT32_MAX - 1, 0, 0, 0.0f);
		att.load_op						 = load_op::clear;
		att.store_op					 = store_op::store;
		att.texture						 = render_target;

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_source,
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

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "object_id_pass");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = &att,
														   .depth_stencil_attachment =
															   {
																   .texture		   = p.depth_texture,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = 1,
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

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_copy_source,
			.resource	 = render_target,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });
		barriers.resize(0);

		backend->cmd_copy_texture_to_buffer(cmd_buffer,
											{
												.dest_buffer = readback,
												.src_texture = render_target,
												.src_layer	 = 0,
												.src_mip	 = 0,
												.size		 = vector2ui(p.size.x, p.size.y),
												.bpp		 = 4,
											});

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_object_id::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	u32 render_pass_object_id::read_location(u16 x, u16 y, u8 frame_index)
	{
		per_frame_data& pfd		   = _pfd[frame_index];
		gfx_backend*	backend	   = gfx_backend::get();
		const u32		aligned	   = backend->align_texture_size_pitch(_size.x * 4);
		const u32		data_index = aligned * y + x * 4;

		const u32 pixel = _size.x * y + x;
		u8*		  data	= reinterpret_cast<u8*>(pfd.readback_mapped);
		u32		  val	= *reinterpret_cast<u32*>(&data[data_index]);
		return val;
	}

	void render_pass_object_id::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
			backend->destroy_resource(pfd.readback_buffer);
			pfd.render_target = NULL_GFX_ID;
		}
	}

	void render_pass_object_id::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();
		_size				 = sz;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.render_target	= backend->create_texture({
				  .texture_format = render_target_definitions::get_format_object_id(),
				  .size			  = sz,
				  .flags		  = texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				  .views		  = {{.type = view_type::render_target}},
				  .clear_values	  = {static_cast<f32>(UINT32_MAX - 1), 0, 0, 0},
				  .debug_name	  = "object_id_rt",
			  });

			const u32 aligned = backend->align_texture_size_pitch(sz.x * 4);

			pfd.readback_buffer = backend->create_resource({.size = static_cast<u32>(aligned * sz.y), .flags = resource_flags::rf_readback, .debug_name = "object_id_readback"});
			backend->map_resource(pfd.readback_buffer, pfd.readback_mapped);
		}
	}
}
