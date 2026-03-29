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

#include "render_pass_canvas_2d.hpp"
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
#include "gui/vekt.hpp"
#include "common/system_info.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_canvas_2d::init(const vector2ui16& size)
	{
		_alloc.init(PASS_ALLOC_SIZE_CANVAS_2D, 8);

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "canvas_2d_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "canvas_2d_ubo"});
		}
	}

	void render_pass_canvas_2d::uninit()
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

	void render_pass_canvas_2d::prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_CANVAS_2D);

		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		gfx_backend* backend = gfx_backend::get();
		backend->reset_command_buffer(cmd_buffer);

		auto*	  canvases		= pm.get_canvases();
		auto*	  materials		= pm.get_material_runtimes();
		auto*	  entities		= pm.get_entities();
		const u32 peak_canvases = pm.get_peak_canvases();

		// -----------------------------------------------------------------------------
		// record barriers & draw calls
		// -----------------------------------------------------------------------------

		static_vector<barrier, 24> barriers;
		static_vector<barrier, 24> barriers_after;

		for (u32 i = 0; i < peak_canvases; i++)
		{
			render_proxy_canvas& proxy = canvases->get(i);
			if (proxy.status != render_proxy_status::rps_active)
				continue;

			if (proxy.is_3d)
				continue;

			const render_proxy_entity& entity = entities->get(proxy.entity);
			SFG_ASSERT(entity.status == render_proxy_status::rps_active);

			if (entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			const gfx_id vb = proxy.vertex_buffers[frame_index].get_gpu();
			const gfx_id ib = proxy.index_buffers[frame_index].get_gpu();

			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = vb,
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = ib,
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers_after.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = vb,
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers_after.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = ib,
				.flags		 = barrier_flags::baf_is_resource,
			});

			for (const render_proxy_canvas_dc& dc : proxy._draw_calls)
			{
				render_proxy_material_runtime& mat = materials->get(dc.mat_id);

				const gfx_id	pipeline   = pm.get_shader_variant(mat.shader_handle, 0);
				const gpu_index mat_const  = mat.gpu_index_buffers[frame_index];
				const gpu_index txt_const  = mat.gpu_index_texture_buffers[frame_index];
				gpu_index		font_index = NULL_GPU_INDEX;

				if (dc.atlas_exists)
				{
					render_proxy_texture& txt = pm.get_texture(dc.atlas_id);
					SFG_ASSERT(txt.status == render_proxy_status::rps_active);
					font_index = txt.heap_index;
				}

				_draw_stream.add_command({
					.clip					 = dc.clip,
					.start_index			 = dc.start_index,
					.index_count			 = dc.index_count,
					.base_vertex			 = dc.start_vertex,
					.material_constant_index = mat_const,
					.texture_constant_index	 = txt_const,
					.font_index				 = font_index,
					.vb_hw					 = vb,
					.ib_hw					 = ib,
					.pipeline_hw			 = pipeline,
					.vertex_size			 = sizeof(vekt::vertex),
					.idx_size				 = sizeof(vekt::index),
				});
			}
		}

		// -----------------------------------------------------------------------------
		// apply the barriers & perform the copies
		// -----------------------------------------------------------------------------

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		// Copy regions
		for (u32 i = 0; i < peak_canvases; i++)
		{
			render_proxy_canvas& proxy = canvases->get(i);
			if (proxy.status != render_proxy_status::rps_active)
				continue;

			if (proxy.is_3d)
				continue;

			const render_proxy_entity& entity = entities->get(proxy.entity);
			SFG_ASSERT(entity.status == render_proxy_status::rps_active);

			if (entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			proxy.vertex_buffers[frame_index].copy_region(cmd_buffer, 0, proxy._max_vertex_offset);
			proxy.index_buffers[frame_index].copy_region(cmd_buffer, 0, proxy._max_index_offset);
		}

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers_after.data(),
								 .barrier_count = static_cast<u16>(barriers_after.size()),
							 });

		const ubo ubo_data = {
			.proj		= matrix4x4::ortho_reverse_z(0, static_cast<f32>(resolution.x), 0, static_cast<f32>(resolution.y), 0.0f, 1.0f),
			.resolution = vector2(static_cast<f32>(resolution.x), static_cast<f32>(resolution.y)),
		};
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_canvas_2d::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();
		const gfx_id	target_texture	 = p.input_texture;

		render_pass_color_attachment* attachments = _alloc.allocate<render_pass_color_attachment>(1);
		render_pass_color_attachment& att		  = attachments[0];
		att.clear_color							  = vector4(0, 0, 0, 1.0f);
		att.load_op								  = load_op::load;
		att.store_op							  = store_op::store;
		att.texture								  = target_texture;
		att.view_index							  = 0;

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "canvas_2d_pass");

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments = attachments,

										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const u32 rp_constants[4] = {gpu_index_rp_ubo};
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

		static_vector<barrier, 1> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = target_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

}
