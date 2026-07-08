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
#include <sfg/math/vec2u16.hpp>

namespace sfg::ui
{
	struct vg_draw_snapshot_t;

	struct ui_renderer_config_t
	{
		u32 vertex_buffer_bytes = 1u << 20;
		u32 index_buffer_bytes	= 1u << 20;
	};

	class ui_renderer_t
	{
	public:
		ui_renderer_t()								   = default;
		~ui_renderer_t()							   = default;
		ui_renderer_t(const ui_renderer_t&)			   = delete;
		ui_renderer_t& operator=(const ui_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const ui_renderer_config_t& cfg = {});
		void uninit();
		void render(gfx_handle_t cmd, const vg_draw_snapshot_t* snap, u8 frame_index, vec2u16_t fb_size);

	private:
		struct per_frame_data_t
		{
			gfx_handle_t vertex_buffer	   = {};
			gfx_handle_t index_buffer	   = {};
			gfx_handle_t projection_buffer = {};
			u8*			 mapped_vtx		   = nullptr;
			u8*			 mapped_idx		   = nullptr;
			u8*			 mapped_projection = nullptr;
			gpu_index_t	 projection_index  = 0;
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		gfx_handle_t	 _sdf_params			 = {};
		gpu_index_t		 _sdf_params_index		 = 0;
		u32				 _vtx_capacity			 = 0;
		u32				 _idx_capacity			 = 0;
	};
}
