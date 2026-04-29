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

#include "ui_renderer.hpp"
#include "ui/vg/vg_canvas.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/descriptions.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "math/math.hpp"

namespace sfg::ui
{
	ui_renderer_t::~ui_renderer_t() = default;

	void ui_renderer_t::init(const ui_render_group_t& default_group, const ui_render_group_t& default_text_group, const ui_render_group_t& default_sdf_group, const ui_renderer_config_t& cfg)
	{
		gfx_backend* backend = gfx_backend::get();

		_default_group		= default_group;
		_default_text_group = default_text_group;
		_default_sdf_group	= default_sdf_group;
		_vtx_capacity		= cfg.vertex_buffer_bytes;
		_idx_capacity		= cfg.index_buffer_bytes;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];

			resource_desc_t v_desc = {};
			v_desc.size			   = _vtx_capacity;
			v_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
			v_desc.debug_name	   = "ui_renderer_vtx";
			p.vertex_buffer		   = backend->create_resource(v_desc);
			backend->map_resource(p.vertex_buffer, p.mapped_vtx);

			resource_desc_t i_desc = {};
			i_desc.size			   = _idx_capacity;
			i_desc.flags		   = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
			i_desc.debug_name	   = "ui_renderer_idx";
			p.index_buffer		   = backend->create_resource(i_desc);
			backend->map_resource(p.index_buffer, p.mapped_idx);
		}
	}

	void ui_renderer_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];
			backend->destroy_resource(p.vertex_buffer);
			backend->destroy_resource(p.index_buffer);
			p = {};
		}

		_default_group		= {};
		_default_text_group = {};
		_default_sdf_group	= {};
		_vtx_capacity		= 0;
		_idx_capacity		= 0;
	}

	void ui_renderer_t::render(gfx_command_buffer_handle cmd, const vg_canvas_t& canvas, u8 frame_index)
	{
		gfx_backend*	  backend = gfx_backend::get();
		per_frame_data_t& p		  = _pfd[frame_index % BACK_BUFFER_COUNT];

		const auto& get_draw_buffers = canvas.get_draw_buffers();
		if (get_draw_buffers.empty())
			return;

		u32 vtx_offset = 0;
		u32 idx_offset = 0;

		ui_render_group_t current = {};

		for (const vg_draw_buffer_t& db : get_draw_buffers)
		{
			if (db.vertex_count == 0 || db.index_count == 0)
				continue;

			const u32 vtx_size = db.vertex_count * sizeof(vg_vertex_t);
			const u32 idx_size = db.index_count * sizeof(vg_index_t);

			if (vtx_offset + vtx_size > _vtx_capacity || idx_offset + idx_size > _idx_capacity)
			{
				SFG_ERR("ui_renderer: per-frame upload buffer exhausted, dropping draw");
				continue;
			}

			const ui_render_group_t* selected = nullptr;
			if (db.user_data != nullptr)
			{
				selected = static_cast<const ui_render_group_t*>(db.user_data);
			}
			else if (db.font_id == invalid_id_u32)
			{
				selected = &_default_group;
			}
			else if (db.font_kind == vg_font_kind_e::sdf)
			{
				selected = &_default_sdf_group;
			}
			else
			{
				selected = &_default_text_group;
			}

			if (selected->layout != current.layout)
			{
				command_bind_layout_t bl = {.layout = selected->layout};
				backend->cmd_bind_layout(cmd, bl);
				current.layout = selected->layout;
			}

			if (selected->group != current.group)
			{
				command_bind_group_t bg = {.group = selected->group};
				backend->cmd_bind_group(cmd, bg);
				current.group = selected->group;
			}

			if (selected->pipeline != current.pipeline)
			{
				command_bind_pipeline_t bp = {.pipeline = selected->pipeline};
				backend->cmd_bind_pipeline(cmd, bp);
				current.pipeline = selected->pipeline;
			}

			SFG_MEMCPY(p.mapped_vtx + vtx_offset, db.vertex_start, vtx_size);
			SFG_MEMCPY(p.mapped_idx + idx_offset, db.index_start, idx_size);

			const u16			   sx = static_cast<u16>(math::max(0.0f, db.clip.x));
			const u16			   sy = static_cast<u16>(math::max(0.0f, db.clip.y));
			const u16			   sw = static_cast<u16>(math::max(0.0f, db.clip.z));
			const u16			   sh = static_cast<u16>(math::max(0.0f, db.clip.w));
			command_set_scissors_t sc = {.x = sx, .y = sy, .width = sw, .height = sh};
			backend->cmd_set_scissors(cmd, sc);

			command_bind_vertex_buffers_t vb = {};
			vb.buffer_t						 = p.vertex_buffer;
			vb.slot							 = 0;
			vb.vertex_size					 = sizeof(vg_vertex_t);
			vb.offset						 = vtx_offset;
			backend->cmd_bind_vertex_buffers(cmd, vb);

			command_bind_index_buffers_t ib = {};
			ib.buffer_t						= p.index_buffer;
			ib.offset						= idx_offset;
			ib.index_size					= sizeof(vg_index_t);
			backend->cmd_bind_index_buffers(cmd, ib);

			command_draw_indexed_instanced_t draw_cmd = {};
			draw_cmd.index_count_per_instance		  = db.index_count;
			draw_cmd.instance_count					  = 1;
			draw_cmd.start_index_location			  = 0;
			draw_cmd.base_vertex_location			  = 0;
			draw_cmd.start_instance_location		  = 0;
			backend->cmd_draw_indexed_instanced(cmd, draw_cmd);

			vtx_offset += vtx_size;
			idx_offset += idx_size;
		}
	}
}
