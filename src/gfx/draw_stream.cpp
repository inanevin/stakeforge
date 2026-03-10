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

#include "draw_stream.hpp"
#include "memory/bump_allocator.hpp"
#include "math/math.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "resources/vertex.hpp"
#include "common/system_info.hpp"
#include <algorithm>
#include <tracy/Tracy.hpp>

namespace SFG
{
	void draw_stream::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) {
			return A.priority < B.priority && draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw);
		});
	}

	void draw_stream::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<u8>(sizeof(primitive_index))});

			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.entity_constant_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.bone_constant_index, .offset = constant_index_object_constant1, .count = 1, .param_index = rpi_constants});

#ifdef SFG_TOOLMODE
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.entity_world_id, .offset = constant_index_object_constant2, .count = 1, .param_index = rpi_constants});
#endif

			const u32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.sampler_constant_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}

	void draw_stream::add_command(const draw_command& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_distance::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_distance>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream_distance::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) {
			const f32 da = A.distance, db = B.distance;
			if (A.priority == B.priority)
			{
				if (math::almost_equal(da, db))
					return draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw);
				if (math::is_nan(da))
					return false;
				if (math::is_nan(db))
					return true;
				return da > db; // farther first
			}
			return A.priority < B.priority;
		});
	}

	void draw_stream_distance::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command_distance& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<u8>(sizeof(primitive_index))});

			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.entity_constant_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.bone_constant_index, .offset = constant_index_object_constant1, .count = 1, .param_index = rpi_constants});

#ifdef SFG_TOOLMODE
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&draw.entity_world_id, .offset = constant_index_object_constant2, .count = 1, .param_index = rpi_constants});
#endif

			const u32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.sampler_constant_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}
	void draw_stream_distance::add_command(const draw_command_distance& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_gui::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_gui>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream_gui::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) { return draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw); });
	}

	void draw_stream_gui::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command_gui& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = draw.idx_size});

			backend->cmd_set_scissors(cmd_buffer, {.x = draw.clip.x, .y = draw.clip.y, .width = draw.clip.z, .height = draw.clip.w});

			const u32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.font_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}

	void draw_stream_gui::draw_no_clip(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command_gui& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = draw.idx_size});

			const u32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.font_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}

	void draw_stream_gui::add_command(const draw_command_gui& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_particle::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_particle>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}
	void draw_stream_particle::build()
	{
		ZoneScoped;
		std::stable_sort(_commands, _commands + _commands_count, [&](const draw_command_particle& a, const draw_command_particle& b) {
			return draw_stream_bound_state_pipeline::make_sort_key(a.pipeline_hw) < draw_stream_bound_state_pipeline::make_sort_key(b.pipeline_hw);
		});
	}
	void draw_stream_particle::draw(gfx_id cmd_buffer, gfx_id indirect_buffer, gfx_id indirect_signature, u32 indirect_buffer_size, u32 max_instances_per_system)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state_pipeline bound = {};

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command_particle& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw);
			if (diff)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			const u32 mat_constants[2] = {draw.material_index, draw.texture_buffer_index};
			const u32 rp_constant	   = draw.system_index * max_instances_per_system;
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&rp_constant, .offset = constant_index_rp_constant2, .count = 1, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 2, .param_index = rpi_constants});

			backend->cmd_execute_indirect(cmd_buffer,
										  {
											  .indirect_buffer		  = indirect_buffer,
											  .indirect_buffer_offset = indirect_buffer_size * draw.system_index,
											  .count				  = 1,
											  .indirect_signature	  = indirect_signature,
										  });
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}
	void draw_stream_particle::add_command(const draw_command_particle& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_sprite::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_sprite>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream_sprite::build()
	{
		ZoneScoped;
		std::stable_sort(
			_commands, _commands + _commands_count, [&](const draw_command_sprite& a, const draw_command_sprite& b) { return draw_stream_bound_state_pipeline::make_sort_key(a.pipeline_hw) < draw_stream_bound_state_pipeline::make_sort_key(b.pipeline_hw); });
	}

	void draw_stream_sprite::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state_pipeline bound = {};

		u32 icounter = 0;

		for (u32 i = 0; i < _commands_count; i++)
		{
			draw_command_sprite& draw = _commands[i];

			const u8 diff = bound.diff_mask(draw.pipeline_hw);
			if (diff)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			const u32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.sampler_constant_index};
			const u32 rp_constant	   = icounter;
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&rp_constant, .offset = constant_index_rp_constant3, .count = 1, .param_index = rpi_constants});
			icounter += draw.instance_count;

			backend->cmd_draw_instanced(cmd_buffer,
										{
											.vertex_count_per_instance = 4,
											.instance_count			   = draw.instance_count,
											.start_vertex_location	   = 0,
											.start_instance_location   = draw.start_instance,
										});
		}

#ifdef SFG_TOOLMODE
		g_frame_info.draw_calls += _commands_count;
#endif
	}

	void draw_stream_sprite::add_command(const draw_command_sprite& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}
}
