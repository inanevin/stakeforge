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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class atlas_t;
	class texture_queue_t;
}

namespace sfg::ui
{
	class vg_canvas_t;

	struct ui_render_group_t
	{
		gfx_shader_handle pipeline	   = {};
		u32				  constants[4] = {};
	};

	struct ui_renderer_config_t
	{
		u32 vertex_buffer_bytes = 1u << 20;
		u32 index_buffer_bytes	= 1u << 20;
	};

	class ui_renderer_t
	{
	public:
		ui_renderer_t()								   = default;
		ui_renderer_t(const ui_renderer_t&)			   = delete;
		ui_renderer_t& operator=(const ui_renderer_t&) = delete;
		~ui_renderer_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(gfx_shader_handle default_pipeline, gfx_shader_handle text_pipeline, gfx_shader_handle sdf_pipeline, const ui_renderer_config_t& cfg = {});
		void uninit();
		void render(gfx_command_buffer_handle cmd, const vg_canvas_t& canvas, u8 frame_index, vec2u16_t fb_size);

		// -----------------------------------------------------------------------------
		// atlas
		// -----------------------------------------------------------------------------

		void update_atlas(texture_queue_t& queue, atlas_t* atlas);

	private:
		struct per_frame_data_t
		{
			gfx_resource_handle vertex_buffer	  = {};
			gfx_resource_handle index_buffer	  = {};
			gfx_resource_handle projection_buffer = {};
			u8*					mapped_vtx		  = nullptr;
			u8*					mapped_idx		  = nullptr;
			u8*					mapped_projection = nullptr;
			u32					projection_index  = 0;
		};

		struct atlas_entry_t
		{
			gfx_texture_handle	texture		 = {};
			gfx_resource_handle staging		 = {};
			u32					gpu_index	 = 0;
			u32					width		 = 0;
			u32					height		 = 0;
			u8					bpp			 = 1;
			bool				transitioned = false;
		};

	private:
		per_frame_data_t			   _pfd[BACK_BUFFER_COUNT] = {};
		hash_map_t<u32, atlas_entry_t> _atlases;
		gfx_shader_handle			   _default_pipeline = {};
		gfx_shader_handle			   _text_pipeline	 = {};
		gfx_shader_handle			   _sdf_pipeline	 = {};
		gfx_resource_handle			   _sdf_params		 = {};
		u32							   _sdf_params_index = 0;
		u32							   _vtx_capacity	 = 0;
		u32							   _idx_capacity	 = 0;
	};
}
