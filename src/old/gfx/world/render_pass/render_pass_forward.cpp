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

#include "render_pass_forward.hpp"
#include "math/math.hpp"

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
	void render_pass_forward::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_FORWARD, 8);

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "forward_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "forward_ubo"});
		}
	}

	void render_pass_forward::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
		}
	}

	void render_pass_forward::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_FORWARD);

		renderable_collector::populate_draw_stream(pm, renderables, _draw_stream, material_flags::material_flags_is_forward, 0, frame_index);

		const u8					ambient_exists = pm.get_ambient_exists();
		const render_proxy_ambient& ambient		   = pm.get_ambient();
		const vector3				ambient_color  = ambient_exists ? ambient.base_color : vector3(0.1f, 0.1f, 0.1f);

		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .view_proj   = main_camera_view.view_proj_matrix,
				  .ambient	   = vector4(ambient_color.x, ambient_color.y, ambient_color.z, 1.0f),
				  .camera_pos  = vector4(main_camera_view.position.x, main_camera_view.position.y, main_camera_view.position.z, 1.0f),
				  .resolution  = vector2(static_cast<f32>(resolution.x), static_cast<f32>(resolution.y)),
				  .proj_params = vector2(math::tan(0.5f * main_camera_view.fov_degrees * DEG_2_RAD), 0.0f),
		  };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_forward::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = p.input_texture,
			.load_op	 = load_op::load,
			.store_op	 = store_op::store,
		};

		backend->reset_command_buffer(cmd_buffer);

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "forward_pass");

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

		const u32 rp_constants[4] = {gpu_index_rp_ubo, p.gpu_index_entities, p.gpu_index_bones, p.gpu_index_dir_lights};
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 4, .param_index = rpi_constants});

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

		if (p.transition)
		{

			static_vector<barrier, 2> barriers;

			barriers.push_back({
				.from_states = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
				.to_states	 = resource_state::resource_state_common,
				.resource	 = p.depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_render_target,
				.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
				.resource	 = p.input_texture,
				.flags		 = barrier_flags::baf_is_texture,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<u16>(barriers.size()),
								 });
		}

		backend->close_command_buffer(cmd_buffer);
	}

}
