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

#include "world_rendering.hpp"
#include "world_render_context.hpp"
#include "world_snapshot.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>

namespace sfg
{
	void world_rendering_t::render_world(const world_render_context_t& ctx, const world_snapshot_t&, u8 frame_index)
	{
		gfx_backend& backend = gfx_backend::get();

		const gfx_command_buffer_handle cmd			  = ctx.get_command_buffer(frame_index);
		const gfx_texture_handle		color_texture = ctx.get_world_texture(frame_index);
		const gfx_texture_handle		depth_texture = ctx.get_depth_texture(frame_index);

		backend.reset_command_buffer(cmd);

		barrier_t begin_barriers[2] = {};
		u16		  begin_count		= 0;

		const u32 color_state = backend.get_texture_state(color_texture);
		if (color_state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = color_state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = color_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		const u32 depth_state = backend.get_texture_state(depth_texture);
		if (depth_state != resource_state_depth_write)
		{
			begin_barriers[begin_count++] = {
				.from_states = depth_state,
				.to_states	 = resource_state_depth_write,
				.texture_t	 = depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
			backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture_t	 = color_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 1,
		};

		render_pass_depth_stencil_attachment_t depth_attachment = {
			.texture_t		  = depth_texture,
			.clear_stencil	  = 0,
			.clear_depth	  = 0.0f,
			.depth_load_op	  = load_op::clear,
			.stencil_load_op  = load_op::none,
			.depth_store_op	  = store_op::store,
			.stencil_store_op = store_op::none,
			.view_index		  = 0,
		};

		backend.cmd_begin_render_pass_depth(cmd,
											{
												.color_attachments		  = &color_attachment,
												.depth_stencil_attachment = depth_attachment,
												.color_attachment_count	  = 1,
											});

		const vec2u16_t size = ctx.get_size();
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		backend.cmd_end_render_pass(cmd, {});

		const barrier_t end_barriers[2] = {
			{
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = color_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
			{
				.from_states = resource_state_depth_write,
				.to_states	 = resource_state_depth_read,
				.texture_t	 = depth_texture,
				.flags		 = barrier_flags::baf_is_texture,
			},
		};
		backend.cmd_barrier(cmd, {.barriers = end_barriers, .barrier_count = 2});
		backend.close_command_buffer(cmd);
	}
}
