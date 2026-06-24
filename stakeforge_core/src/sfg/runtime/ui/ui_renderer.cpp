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
#include "ui_context.hpp"
#include "vg/vg_canvas.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg::ui
{
	namespace
	{
		struct sdf_params_data_t
		{
			f32 sdf_threshold = 0.5f;
			f32 sdf_softness  = 0.0625f;
			f32 _pad0		  = 0.0f;
			f32 _pad1		  = 0.0f;
		};

		inline u16 scissor_pos(f32 v)
		{
			return static_cast<u16>(math::max(0.0f, math::floor(v)));
		}

		inline u16 scissor_size(f32 min_v, f32 size_v)
		{
			const f32 min_px = math::max(0.0f, math::floor(min_v));
			const f32 max_px = math::max(min_px, math::ceil(min_v + size_v));
			return static_cast<u16>(max_px - min_px);
		}

		gpu_index_t resolve_constant(const ui_resolved_resource_ref_t& ref, u8 frame_slot)
		{
			if (ref.type == ui_resource_type_e::gpu_index)
				return ref.gpu_indices[0];
			if (ref.type == ui_resource_type_e::gpu_index_fof)
				return ref.gpu_indices[frame_slot];
			if (ref.type == ui_resource_type_e::texture)
				return render_resources_t::get().get_texture_gpu_index(ref.texture, 0);
			return NULL_GPU_INDEX;
		}
	}

	void ui_renderer_t::init(const ui_renderer_config_t& cfg)
	{
		gfx_backend& backend = gfx_backend::get();

		_vtx_capacity = cfg.vertex_buffer_bytes;
		_idx_capacity = cfg.index_buffer_bytes;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];

			resource_desc_t v_desc = {};
			v_desc.size			   = _vtx_capacity;
			v_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
			v_desc.set_name("ui_renderer_vtx");
			p.vertex_buffer = backend.create_resource(v_desc);
			backend.map_resource(p.vertex_buffer, p.mapped_vtx);

			resource_desc_t i_desc = {};
			i_desc.size			   = _idx_capacity;
			i_desc.flags		   = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
			i_desc.set_name("ui_renderer_idx");
			p.index_buffer = backend.create_resource(i_desc);
			backend.map_resource(p.index_buffer, p.mapped_idx);

			resource_desc_t proj_desc = {};
			proj_desc.size			  = sizeof(f32) * 16;
			proj_desc.flags			  = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			proj_desc.set_name("ui_renderer_projection");
			p.projection_buffer = backend.create_resource(proj_desc);
			backend.map_resource(p.projection_buffer, p.mapped_projection);
			p.projection_index = backend.get_resource_gpu_index(p.projection_buffer);
		}

		resource_desc_t sdf_desc = {};
		sdf_desc.size			 = sizeof(sdf_params_data_t);
		sdf_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		sdf_desc.set_name("ui_renderer_sdf_params");
		_sdf_params		  = backend.create_resource(sdf_desc);
		_sdf_params_index = backend.get_resource_gpu_index(_sdf_params);

		u8* sdf_mapped = nullptr;
		backend.map_resource(_sdf_params, sdf_mapped);
		const sdf_params_data_t defaults = {};
		SFG_MEMCPY(sdf_mapped, &defaults, sizeof(sdf_params_data_t));
		backend.unmap_resource(_sdf_params);
	}

	void ui_renderer_t::uninit()
	{
		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];
			backend.destroy_resource(p.vertex_buffer);
			backend.destroy_resource(p.index_buffer);
			backend.destroy_resource(p.projection_buffer);
			p = {};
		}

		backend.destroy_resource(_sdf_params);

		_sdf_params		  = {};
		_sdf_params_index = 0;
		_vtx_capacity	  = 0;
		_idx_capacity	  = 0;
	}

	void ui_renderer_t::render(gfx_handle_t cmd, ui_context& ctx, u8 frame_index, vec2u16_t fb_size)
	{
		gfx_backend&	  backend	 = gfx_backend::get();
		const u8		  frame_slot = frame_index % BACK_BUFFER_COUNT;
		per_frame_data_t& pfd		 = _pfd[frame_slot];

		const vg_draw_snapshot_t* snap = ctx.acquire_render_snapshot();
		if (snap == nullptr || snap->draw_buffer_count == 0)
			return;

		const mat4x4_t proj = mat4x4_t::ortho(0.0f, static_cast<f32>(fb_size.x), 0.0f, static_cast<f32>(fb_size.y), 0.0f, 1.0f);
		SFG_MEMCPY(pfd.mapped_projection, proj.m, sizeof(f32) * 16);

		command_bind_constants_t bc_proj = {.data = &pfd.projection_index, .offset = constant_rp0, .count = 1, .param_index = 0};
		backend.cmd_bind_constants(cmd, bc_proj);

		u32 vtx_offset = 0;
		u32 idx_offset = 0;

		gfx_handle_t current_pipeline = {};

		for (u32 i = 0; i < snap->draw_buffer_count; ++i)
		{
			const vg_draw_buffer_final_t& db = snap->draw_buffers[i];

			if (db.vertex_count == 0 || db.index_count == 0)
				continue;

			const u32 vtx_size = db.vertex_count * sizeof(vg_vertex_t);
			const u32 idx_size = db.index_count * sizeof(vg_index_t);

			if (vtx_offset + vtx_size > _vtx_capacity || idx_offset + idx_size > _idx_capacity)
			{
				SFG_ERR("per-frame upload buffer exhausted, dropping draw");
				continue;
			}

			if (db.resolved.pipeline.is_null())
				continue;

			const gfx_handle_t pipeline = render_resources_t::get().get_shader_hw(db.resolved.pipeline);

			if (pipeline != current_pipeline)
			{
				command_bind_pipeline_t bp = {.pipeline = pipeline};
				backend.cmd_bind_pipeline(cmd, bp);
				current_pipeline = pipeline;
			}

			glyph_atlas_t&	  glyph_atlas = resource_manager_t::get().get_glyph_atlas();
			const gpu_index_t atlas_index = render_resources_t::get().get_texture_gpu_index(glyph_atlas.get_texture(), 0);

			gpu_index_t				 mat_constants[2] = {atlas_index, _sdf_params_index};
			command_bind_constants_t bc_mat0		  = {.data = mat_constants, .offset = constant_mat0, .count = 2, .param_index = 0};
			backend.cmd_bind_constants(cmd, bc_mat0);

			gpu_index_t obj_constants[4] = {};
			for (u8 j = 0; j < 4; ++j)
			{
				obj_constants[j] = resolve_constant(db.resolved.constants[j], frame_slot);
			}

			command_bind_constants_t bc_obj = {.data = obj_constants, .offset = constant_obj0, .count = 4, .param_index = 0};
			backend.cmd_bind_constants(cmd, bc_obj);

			SFG_MEMCPY(pfd.mapped_vtx + vtx_offset, snap->vertices + db.vertex_offset, vtx_size);
			SFG_MEMCPY(pfd.mapped_idx + idx_offset, snap->indices + db.index_offset, idx_size);

			const u16			   sx = scissor_pos(db.clip.x);
			const u16			   sy = scissor_pos(db.clip.y);
			const u16			   sw = scissor_size(db.clip.x, db.clip.z);
			const u16			   sh = scissor_size(db.clip.y, db.clip.w);
			command_set_scissors_t sc = {.x = sx, .y = sy, .width = sw, .height = sh};
			backend.cmd_set_scissors(cmd, sc);

			command_bind_vertex_buffers_t vb = {};
			vb.buffer_t						 = pfd.vertex_buffer;
			vb.slot							 = 0;
			vb.vertex_size					 = sizeof(vg_vertex_t);
			vb.offset						 = vtx_offset;
			backend.cmd_bind_vertex_buffers(cmd, vb);

			command_bind_index_buffers_t ib = {};
			ib.buffer_t						= pfd.index_buffer;
			ib.offset						= idx_offset;
			ib.index_size					= sizeof(vg_index_t);
			backend.cmd_bind_index_buffers(cmd, ib);

			command_draw_indexed_instanced_t draw_cmd = {};
			draw_cmd.index_count_per_instance		  = db.index_count;
			draw_cmd.instance_count					  = 1;
			draw_cmd.start_index_location			  = 0;
			draw_cmd.base_vertex_location			  = 0;
			draw_cmd.start_instance_location		  = 0;
			backend.cmd_draw_indexed_instanced(cmd, draw_cmd);

			vtx_offset += vtx_size;
			idx_offset += idx_size;
		}
	}
}
