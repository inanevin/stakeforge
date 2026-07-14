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

#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/data/span.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/world_render_context.hpp>

namespace sfg
{
	struct world_render_snapshot_t;

	struct editor_world_snapshot_data_t
	{
	};

	class editor_world_renderer_t final
	{
	public:
		editor_world_renderer_t()										   = default;
		~editor_world_renderer_t()										   = default;
		editor_world_renderer_t(const editor_world_renderer_t&)			   = delete;
		editor_world_renderer_t& operator=(const editor_world_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(vec2u16_t size, span_t<world_render_snapshot_t> snapshots);
		void uninit(span_t<world_render_snapshot_t> snapshots);
		void resize(vec2u16_t size);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index_t get_world_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].world_texture_index;
		}

	private:
		void create_texture(vec2u16_t size);
		void destroy_texture();

		struct per_frame_data_t
		{
			gfx_handle_t cmd_gfx			 = {};
			gfx_handle_t world_texture		 = {};
			gpu_index_t	 world_texture_index = NULL_GPU_INDEX;
		};

	private:
		per_frame_data_t	   _pfd[BACK_BUFFER_COUNT] = {};
		world_render_context_t _world_render_context   = {};
		gfx_handle_t		   _shader				   = {};
		vec2u16_t			   _size				   = vec2u16_t::zero;
	};
}
