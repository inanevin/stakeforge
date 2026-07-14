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
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class editor_world_render_context_t final
	{
	public:
		editor_world_render_context_t()												   = default;
		~editor_world_render_context_t()											   = default;
		editor_world_render_context_t(const editor_world_render_context_t&)			   = delete;
		editor_world_render_context_t& operator=(const editor_world_render_context_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(vec2u16_t size);
		void uninit();
		void resize(vec2u16_t size);

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
			gfx_handle_t world_texture		 = {};
			gpu_index_t	 world_texture_index = NULL_GPU_INDEX;
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		vec2u16_t		 _size					 = vec2u16_t::zero;
	};
}
