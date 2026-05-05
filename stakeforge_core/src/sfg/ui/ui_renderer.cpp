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
#include "vg/vg_canvas.hpp"
#include <sfg/runtime/resources/atlas.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/mat4x4.hpp>

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
	}

	ui_renderer_t::~ui_renderer_t() = default;

	void ui_renderer_t::init(gfx_shader_handle default_pipeline, gfx_shader_handle text_pipeline, gfx_shader_handle sdf_pipeline, const ui_renderer_config_t& cfg)
	{
		gfx_backend& backend = gfx_backend::get();

		_default_pipeline = default_pipeline;
		_text_pipeline	  = text_pipeline;
		_sdf_pipeline	  = sdf_pipeline;
		_vtx_capacity	  = cfg.vertex_buffer_bytes;
		_idx_capacity	  = cfg.index_buffer_bytes;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];

			resource_desc_t v_desc = {};
			v_desc.size			   = _vtx_capacity;
			v_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
			v_desc.debug_name	   = "ui_renderer_vtx";
			p.vertex_buffer		   = backend.create_resource(v_desc);
			backend.map_resource(p.vertex_buffer, p.mapped_vtx);

			resource_desc_t i_desc = {};
			i_desc.size			   = _idx_capacity;
			i_desc.flags		   = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
			i_desc.debug_name	   = "ui_renderer_idx";
			p.index_buffer		   = backend.create_resource(i_desc);
			backend.map_resource(p.index_buffer, p.mapped_idx);

			resource_desc_t proj_desc = {};
			proj_desc.size			  = sizeof(f32) * 16;
			proj_desc.flags			  = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			proj_desc.debug_name	  = "ui_renderer_projection";
			p.projection_buffer		  = backend.create_resource(proj_desc);
			backend.map_resource(p.projection_buffer, p.mapped_projection);
			p.projection_index = backend.get_resource_gpu_index(p.projection_buffer);
		}

		resource_desc_t sdf_desc = {};
		sdf_desc.size			 = sizeof(sdf_params_data_t);
		sdf_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		sdf_desc.debug_name		 = "ui_renderer_sdf_params";
		_sdf_params				 = backend.create_resource(sdf_desc);
		_sdf_params_index		 = backend.get_resource_gpu_index(_sdf_params);

		u8* sdf_mapped = nullptr;
		backend.map_resource(_sdf_params, sdf_mapped);
		const sdf_params_data_t defaults = {};
		SFG_MEMCPY(sdf_mapped, &defaults, sizeof(sdf_params_data_t));
		backend.unmap_resource(_sdf_params);
	}

	void ui_renderer_t::uninit()
	{
		gfx_backend& backend = gfx_backend::get();

		for (auto& kv : _atlases)
		{
			backend.destroy_texture(kv.second.texture);
			backend.destroy_resource(kv.second.staging);
		}
		_atlases.clear();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			per_frame_data_t& p = _pfd[i];
			backend.destroy_resource(p.vertex_buffer);
			backend.destroy_resource(p.index_buffer);
			backend.destroy_resource(p.projection_buffer);
			p = {};
		}

		backend.destroy_resource(_sdf_params);

		_default_pipeline = {};
		_text_pipeline	  = {};
		_sdf_pipeline	  = {};
		_sdf_params		  = {};
		_sdf_params_index = 0;
		_vtx_capacity	  = 0;
		_idx_capacity	  = 0;
	}

	void ui_renderer_t::update_atlas(texture_queue_t& queue, atlas_t* atlas)
	{
		gfx_backend& backend = gfx_backend::get();

		const u32  atlas_id = atlas->get_id();
		const u32  width	= atlas->get_width();
		const u32  height	= atlas->get_height();
		const bool is_lcd	= atlas->get_is_lcd();
		const u8   bpp		= is_lcd ? 3u : 1u;

		atlas_entry_t& entry = _atlases[atlas_id];

		const bool needs_recreate = entry.texture.is_null() || entry.width != width || entry.height != height || entry.bpp != bpp;

		if (needs_recreate)
		{
			if (!entry.texture.is_null())
				backend.destroy_texture(entry.texture);
			if (!entry.staging.is_null())
				backend.destroy_resource(entry.staging);

			texture_desc_t desc = {};
			desc.texture_format = is_lcd ? format_e::r8g8b8a8_unorm : format_e::r8_unorm;
			desc.size			= {static_cast<u16>(width), static_cast<u16>(height)};
			desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
			desc.mip_levels		= 1;
			desc.array_length	= 1;
			desc.samples		= 1;
			desc.debug_name		= "ui_atlas";
			entry.texture		= backend.create_texture(desc);
			entry.gpu_index		= backend.get_texture_gpu_index(entry.texture, 0);

			resource_desc_t s_desc = {};
			s_desc.size			   = backend.align_texture_size(width * height * bpp);
			s_desc.flags		   = resource_flags::rf_cpu_visible;
			s_desc.debug_name	   = "ui_atlas_staging";
			entry.staging		   = backend.create_resource(s_desc);

			entry.width		   = width;
			entry.height	   = height;
			entry.bpp		   = bpp;
			entry.transitioned = false;
		}

		if (!atlas->is_dirty())
			return;

		const texture_buffer_t mip = {
			.pixels = atlas->get_data(),
			.size	= {static_cast<u16>(width), static_cast<u16>(height)},
			.bpp	= bpp,
		};

		texture_upload_desc_t upload = {};
		upload.texture				 = entry.texture;
		upload.staging				 = entry.staging;
		upload.mips					 = {&mip, 1};
		upload.from_states			 = entry.transitioned ? resource_state_ps_resource : resource_state_common;
		upload.to_states			 = resource_state_ps_resource;
		upload.ownership			 = texture_data_ownership_e::none;
		queue.add(upload);

		entry.transitioned = true;
		atlas->clear_dirty();
	}

	void ui_renderer_t::render(gfx_command_buffer_handle cmd, const vg_canvas_t& canvas, u8 frame_index, vec2u16_t fb_size)
	{
		gfx_backend&	  backend = gfx_backend::get();
		per_frame_data_t& p		  = _pfd[frame_index % BACK_BUFFER_COUNT];

		const auto& draw_buffers = canvas.get_draw_buffers();
		if (draw_buffers.empty())
			return;

		const mat4x4_t proj = mat4x4_t::ortho(0.0f, static_cast<f32>(fb_size.x), 0.0f, static_cast<f32>(fb_size.y), 0.0f, 1.0f);
		SFG_MEMCPY(p.mapped_projection, proj.m, sizeof(f32) * 16);

		command_bind_constants_t bc_proj = {.data = &p.projection_index, .offset = constant_rp0, .count = 1, .param_index = 0};
		backend.cmd_bind_constants(cmd, bc_proj);

		u32 vtx_offset = 0;
		u32 idx_offset = 0;

		gfx_shader_handle current_pipeline = {};

		for (const vg_draw_buffer_t& db : draw_buffers)
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

			const ui_render_group_t* user_group = static_cast<const ui_render_group_t*>(db.user_data);
			const bool				 is_text	= db.font_id != INVALID_ID_U32;
			const bool				 is_sdf		= is_text && db.font_kind == font_kind_e::sdf;

			gfx_shader_handle pipeline = {};
			if (user_group != nullptr)
				pipeline = user_group->pipeline;
			else if (!is_text)
				pipeline = _default_pipeline;
			else if (is_sdf)
				pipeline = _sdf_pipeline;
			else
				pipeline = _text_pipeline;

			if (pipeline != current_pipeline)
			{
				command_bind_pipeline_t bp = {.pipeline = pipeline};
				backend.cmd_bind_pipeline(cmd, bp);
				current_pipeline = pipeline;
			}

			if (is_text)
			{
				u32		   atlas_index = 0;
				const auto it		   = _atlases.find(db.atlas_id);
				if (it != _atlases.end())
					atlas_index = it->second.gpu_index;

				command_bind_constants_t bc_mat0 = {.data = &atlas_index, .offset = constant_mat0, .count = 1, .param_index = 0};
				backend.cmd_bind_constants(cmd, bc_mat0);

				if (is_sdf)
				{
					command_bind_constants_t bc_mat1 = {.data = &_sdf_params_index, .offset = constant_mat1, .count = 1, .param_index = 0};
					backend.cmd_bind_constants(cmd, bc_mat1);
				}
			}

			if (user_group != nullptr)
			{
				command_bind_constants_t bc_obj = {.data = const_cast<u32*>(user_group->constants), .offset = constant_obj0, .count = 4, .param_index = 0};
				backend.cmd_bind_constants(cmd, bc_obj);
			}

			SFG_MEMCPY(p.mapped_vtx + vtx_offset, db.vertex_start, vtx_size);
			SFG_MEMCPY(p.mapped_idx + idx_offset, db.index_start, idx_size);

			const u16			   sx = static_cast<u16>(math::max(0.0f, db.clip.x));
			const u16			   sy = static_cast<u16>(math::max(0.0f, db.clip.y));
			const u16			   sw = static_cast<u16>(math::max(0.0f, db.clip.z));
			const u16			   sh = static_cast<u16>(math::max(0.0f, db.clip.w));
			command_set_scissors_t sc = {.x = sx, .y = sy, .width = sw, .height = sh};
			backend.cmd_set_scissors(cmd, sc);

			command_bind_vertex_buffers_t vb = {};
			vb.buffer_t						 = p.vertex_buffer;
			vb.slot							 = 0;
			vb.vertex_size					 = sizeof(vg_vertex_t);
			vb.offset						 = vtx_offset;
			backend.cmd_bind_vertex_buffers(cmd, vb);

			command_bind_index_buffers_t ib = {};
			ib.buffer_t						= p.index_buffer;
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
