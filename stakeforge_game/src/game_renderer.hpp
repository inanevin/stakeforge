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

#include <sfg/data/atomic.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/math/vec2u16.hpp>

#include <thread>

namespace sfg
{
	struct window_runtime_t;

	struct game_renderer_config_t
	{
		size_t frame_budget_bytes = 4ull * 1024ull * 1024ull;
		bool   is_fullscreen	  = false;
	};

	class game_renderer_t final
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t semaphore_frame	= {};
			semaphore_data_t semaphore_transfer = {};
			gfx_handle_t	 cmd_gfx			= {};
			gfx_handle_t	 cmd_gfx_prepare	= {};
			gfx_handle_t	 cmd_gfx_transit	= {};
			gfx_handle_t	 cmd_transfer		= {};
		};

	public:
		game_renderer_t()								   = default;
		~game_renderer_t()								   = default;
		game_renderer_t(const game_renderer_t&)			   = delete;
		game_renderer_t& operator=(const game_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(window_runtime_t& window, const game_renderer_config_t& config = {});
		void uninit();
		void start();
		void end_render();
		void join();

		// -----------------------------------------------------------------------------
		// swapchain
		// -----------------------------------------------------------------------------

		void resize(vec2u16_t size, f32 dpi_scale, bool minimized);

	private:
		void render();
		void render_loop();

		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		std::thread		 _render_thread;
		gfx_handle_t	 _swapchain			   = {};
		gfx_handle_t	 _blit_shader		   = {};
		vec2u16_t		 _size				   = vec2u16_t::zero;
		size_t			 _frame_budget_bytes   = 0;
		u64				 _frame_counter		   = 0;
		atomic_t<bool>	 _render_thread_active = false;
		bool			 _is_fullscreen		   = false;
		bool			 _minimized			   = false;
	};
}
