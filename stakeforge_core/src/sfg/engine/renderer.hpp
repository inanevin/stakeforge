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

#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"

namespace sfg
{
	class renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t semaphore_frame = {};
		};

	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		u8	 init();
		void shutdown();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void render();

		gfx_id_t create_swapchain(const vec2u16_t& size, format_t format, void* window_handle, void* platform_handle);
		void	 destroy_swapchain(gfx_id_t id);
		void	 resize_swapchain(gfx_id_t id, const vec2u16_t& size);

		gfx_id_t create_render_target(const vec2u16_t& size, format_t& format);
		void	 destroy_render_target(gfx_id_t id);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id_t get_global_bind_layout() const
		{
			return _global_bind_layout;
		}

		inline gfx_id_t get_global_compute_bind_layout() const
		{
			return _global_compute_bind_layout;
		}

		inline u8 get_frame_index() const
		{
			return _frame_index;
		}

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT];
		gfx_id_t		 _global_bind_layout		 = NULL_GFX_ID;
		gfx_id_t		 _global_compute_bind_layout = NULL_GFX_ID;
		u64				 _frame_counter				 = 0;
		u8				 _frame_index				 = 0;
	};
}
