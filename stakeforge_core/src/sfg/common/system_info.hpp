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
#include "size_definitions.hpp"
#include "data/atomic.hpp"

#include <thread>

#ifdef SFG_DEBUG
#include "io/assert.hpp"
#endif

namespace sfg
{
	struct thread_info_t
	{
		std::thread::id thread_id_render;
		std::thread::id thread_id_main;
		bool			is_init = false;
	};

	extern thread_info_t g_thread_info;

#ifdef SFG_DEBUG
#define SFG_REGISTER_THREAD_MAIN()	 g_thread_info.thread_id_main = std::this_thread::get_id()
#define SFG_REGISTER_THREAD_RENDER() g_thread_info.thread_id_render = std::this_thread::get_id()
#define SFG_SET_INIT(IS_INIT)		 g_thread_info.is_init = (IS_INIT)
#define SFG_VERIFY_THREAD_MAIN()	 SFG_ASSERT(g_thread_info.thread_id_main == std::this_thread::get_id())
#define SFG_VERIFY_THREAD_RENDER()	 SFG_ASSERT(g_thread_info.thread_id_render == std::this_thread::get_id())
#define SFG_VERIFY_INIT()			 SFG_ASSERT(g_thread_info.is_init)
#define IS_RENDER_THREAD()			 g_thread_info.thread_id_main == std::this_thread::get_id()
#else
#define SFG_REGISTER_THREAD_MAIN()
#define SFG_REGISTER_THREAD_RENDER()
#define SFG_SET_INIT(IS_INIT)
#define SFG_VERIFY_THREAD_MAIN()
#define SFG_VERIFY_THREAD_RENDER()
#define SFG_VERIFY_INIT()
#endif

	struct frame_info_t
	{
		atomic_t<double> main_thread_time_milli		   = 0;
		atomic_t<double> render_thread_time_milli	   = 0;
		atomic_t<double> render_thread_elapsed_seconds = 0;
		atomic_t<u32>	 fps						   = 0;
		atomic_t<u64>	 frame						   = 0;
		atomic_t<u64>	 render_frame				   = 0;
		u32				 draw_calls					   = 0;
		atomic_t<u32>	 draw_calls_ui				   = 0;
		bool			 is_render_active			   = false;
	};

	extern frame_info_t g_frame_info;

#define SFG_VERIFY_RENDER_NOT_RUNNING()					 SFG_ASSERT(!g_frame_info.is_render_active)
#define SFG_VERIFY_RENDER_THREAD()						 SFG_ASSERT(g_frame_info.is_render_active)
#define SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD() SFG_ASSERT(g_thread_info.thread_id_render == std::this_thread::get_id() || !g_frame_info.is_render_active)
}
