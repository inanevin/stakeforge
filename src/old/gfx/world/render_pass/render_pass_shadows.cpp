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

#include "render_pass_shadows.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/renderer.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/util/shadow_util.hpp"
#include "gfx/world/renderable_collector.hpp"

// misc
#include "world/world.hpp"
#include "game/game_max_defines.hpp"

// math
#include "math/vector2ui16.hpp"
#include "math/math.hpp"

// resources
#include "resources/shader_raw.hpp"
#include "resources/vertex.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

#define MAX_PASS_COUNT 64

	void render_pass_shadows::init()
	{
		_alloc.init(PASS_ALLOC_SIZE_OPAQUE * MAX_PASS_COUNT, 8);
		_barriers.reserve(64);

		_passes = new pass[MAX_PASS_COUNT];

		gfx_backend* backend = gfx_backend::get();

		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "shadows_cmd"});
		}

		for (u16 i = 0; i < MAX_PASS_COUNT; i++)
		{
			pass& p = _passes[i];

			for (u8 j = 0; j < BACK_BUFFER_COUNT; j++)
			{
				p.ubos[j].create({
					.size		= sizeof(ubo),
					.flags		= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
					.debug_name = "shadows_ubo",
				});
			}
			p.renderables.reserve(1024);
		}
	}

	void render_pass_shadows::uninit()
	{
		_alloc.uninit();

		for (u16 i = 0; i < MAX_PASS_COUNT; i++)
		{
			pass& p = _passes[i];

			for (u8 j = 0; j < BACK_BUFFER_COUNT; j++)
			{
				p.ubos[j].destroy();
			}
		}

		delete[] _passes;

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
		}
	}

	void render_pass_shadows::prepare(proxy_manager& pm, const view& main_view, const vector2ui16& resolution, u8 frame_index)
	{
		_alloc.reset();
		_pass_count = 0;
	}

	void render_pass_shadows::add_pass(const pass_props& props)
	{
		ZoneScoped;

		SFG_ASSERT(_pass_count < MAX_GPU_SHADOW_DATA);

		pass& p			   = _passes[_pass_count];
		p.texture		   = props.texture;
		p.transition_owner = props.transition_owner;
		p.view_index	   = props.view_index;
		p.resolution	   = props.res;
		_pass_count++;

		const matrix4x4 view_proj = props.proj * props.view;

		const ubo rp_data = {
			.view_proj = view_proj,
		};
		p.ubos[props.frame_index].buffer_data(0, &rp_data, sizeof(ubo));

		p.pass_view = {
			.view_frustum		  = frustum::extract(view_proj),
			.view_matrix		  = props.view,
			.proj_matrix		  = props.proj,
			.inv_proj_matrix	  = props.proj.inverse(),
			.view_proj_matrix	  = view_proj,
			.inv_view_proj_matrix = view_proj.inverse(),
			.position			  = props.position,
			.near_plane			  = props.cascade_near,
			.far_plane			  = props.cascade_far,
			.fov_degrees		  = props.fov,
		};

		p.stream.prepare(_alloc, MAX_DRAW_CALLS_OPAQUE);
		p.renderables.resize(0);

		renderable_collector::collect_mesh_instances(props.pm, p.pass_view, p.renderables);
		renderable_collector::populate_draw_stream(props.pm, p.renderables, p.stream, material_flags::material_flags_is_gbuffer, shader_variant_flags::variant_flag_z_prepass | shader_variant_flags::variant_flag_shadow_rendering, props.frame_index);
	}

	void render_pass_shadows::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		// const u32 pass_count	 = static_cast<u32>(_pass_count);
		// const u32 base_count	 = pass_count / (u32)SHADOWS_MAX_CMD_BUFFERS;
		// const u32 extra_count = pass_count % (u32)SHADOWS_MAX_CMD_BUFFERS;
		//
		// const u32 my_count = base_count + (p.cmd_index == 0 ? extra_count : 0);
		// const u32 base	  = p.cmd_index == 0 ? 0 : (base_count * p.cmd_index + extra_count);

		const u32 my_count = _pass_count;
		const u32 base	   = 0;

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		// Start barriers for all passes.
		for (u32 i = 0; i < my_count; i++)
		{
			const pass& pass_data = _passes[base + i];

			if (!pass_data.transition_owner)
				continue;

			const gfx_id target_texture = pass_data.texture;

			_barriers.push_back({
				.from_states = resource_state::resource_state_common,
				.to_states	 = resource_state::resource_state_depth_write,
				.resource	 = target_texture,
				.flags		 = barrier_flags::baf_is_texture,
			});
		}

		if (!_barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= _barriers.data(),
									 .barrier_count = static_cast<u16>(_barriers.size()),
								 });
		}
		_barriers.resize(0);

		// Passes
		for (u32 i = 0; i < my_count; i++)
		{
			pass&			  pass_data		   = _passes[base + i];
			const gfx_id	  target_texture   = pass_data.texture;
			const gpu_index	  gpu_index_rp_ubo = pass_data.ubos[p.frame_index].get_index();
			const vector2ui16 resolution	   = pass_data.resolution;
			const u8		  view_index	   = pass_data.view_index;

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "shadow_pass");

			backend->cmd_begin_render_pass_depth_only(cmd_buffer,
													  {
														  .depth_stencil_attachment =
															  {
																  .texture		  = target_texture,
																  .clear_stencil  = 0,
																  .clear_depth	  = 1.0f,
																  .depth_load_op  = load_op::clear,
																  .depth_store_op = store_op::store,
																  .view_index	  = view_index,
															  },
													  });

			// backend->cmd_bind_group(cmd_buffer, {.group = bind_group});
			backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(resolution.x), .height = static_cast<u16>(resolution.y)});
			backend->cmd_set_viewport(cmd_buffer,
									  {
										  .min_depth = 0.0f,
										  .max_depth = 1.0f,
										  .width	 = static_cast<u16>(resolution.x),
										  .height	 = static_cast<u16>(resolution.y),

									  });

			const u32 rp_constants[3] = {gpu_index_rp_ubo, p.gpu_index_entities, p.gpu_index_bones};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(resolution.x), .height = static_cast<u16>(resolution.y)});
			backend->cmd_set_viewport(cmd_buffer,
									  {
										  .min_depth = 0.0f,
										  .max_depth = 1.0f,
										  .width	 = static_cast<u16>(resolution.x),
										  .height	 = static_cast<u16>(resolution.y),

									  });

			pass_data.stream.build();
			pass_data.stream.draw(cmd_buffer);

			backend->cmd_end_render_pass(cmd_buffer, {});

			END_DEBUG_EVENT(backend, cmd_buffer);
		}

		// End barriers for all passes
		for (u32 i = 0; i < my_count; i++)
		{
			const pass&	 pass_data		= _passes[base + i];
			const gfx_id target_texture = pass_data.texture;

			if (!pass_data.transition_owner)
				continue;

			_barriers.push_back({
				.from_states = resource_state::resource_state_depth_write,
				.to_states	 = resource_state::resource_state_common,
				.resource	 = target_texture,
				.flags		 = barrier_flags::baf_is_texture,
			});
		}

		if (!_barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= _barriers.data(),
									 .barrier_count = static_cast<u16>(_barriers.size()),
								 });
		}
		_barriers.resize(0);

		backend->close_command_buffer(cmd_buffer);
	}

}
