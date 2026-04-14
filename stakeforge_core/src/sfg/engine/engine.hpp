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

#include "common/size_definitions.hpp"
#include "engine_window.hpp"
#include "renderer.hpp"
#include "data/atomic.hpp"
#include "memory/dynamic_pool_allocator.hpp"

#include <thread>

namespace sfg
{
	class world_t;
	struct engine_config_t;

	class engine_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		engine_error_code init();
		void			  uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick();
		void start_render();
		void end_render();

		// -----------------------------------------------------------------------------
		// window
		// -----------------------------------------------------------------------------

		engine_id_t create_window(const char* title, const engine_window_state_t& state);
		void		destroy_window(engine_id_t id);

		inline engine_window_state_t& get_window(engine_id_t id)
		{
			return _windows.get(id).state;
		}

		inline const window_runtime_t& get_window_runtime(engine_id_t id) const
		{
			return _windows.get(id).runtime;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline renderer_t& get_renderer()
		{
			return _renderer;
		}

	private:
		void		render();
		static void on_window_event(u32 id, const window_event_t& ev, void* user_data);

	private:
		dynamic_pool_allocator_t<engine_window_t, engine_id_t> _windows;
		std::thread											   _render_thread;
		renderer_t											   _renderer;
		atomic_t<bool>										   _is_init				 = false;
		atomic_t<bool>										   _render_thread_active = false;
		i64													   _previous_time		 = 0;
		i64													   _accumulator_ns		 = 0;
		i64													   _start_time			 = 0;
		i64													   _fps_main_time		 = 0;
		i64													   _fps_render_time		 = 0;
		u32													   _fps_main_frames		 = 0;
		u32													   _fps_render_frames	 = 0;
	};

}
