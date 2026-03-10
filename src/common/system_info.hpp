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

#ifdef SFG_DEBUG
#include <thread>
#include "io/assert.hpp"
#endif

namespace SFG
{
#ifdef SFG_DEBUG

	class thread_info
	{
	public:
		static inline std::thread::id get_thread_id_main()
		{
			return s_thread_id_main;
		}

		static inline std::thread::id get_thread_id_render()
		{
			return s_thread_id_render;
		}

		static inline bool get_is_init()
		{
			return s_is_init;
		}

	private:
		friend class app;

		static inline void SFG_REGISTER_THREAD_MAIN(std::thread::id id)
		{
			s_thread_id_main = id;
		}

		static inline void SFG_REGISTER_THREAD_RENDER(std::thread::id id)
		{
			s_thread_id_render = id;
		}

		static inline void set_is_init(bool is_init)
		{
			s_is_init = is_init;
		}

	private:
		static std::thread::id s_thread_id_render;
		static std::thread::id s_thread_id_main;
		static bool			   s_is_init;
	};

#define SFG_REGISTER_THREAD_MAIN()	 thread_info::SFG_REGISTER_THREAD_MAIN(std::this_thread::get_id())
#define SFG_REGISTER_THREAD_RENDER() thread_info::SFG_REGISTER_THREAD_RENDER(std::this_thread::get_id())
#define SFG_SET_INIT(IS_INIT)		 thread_info::set_is_init(IS_INIT)
#define SFG_VERIFY_THREAD_MAIN()	 SFG_ASSERT(thread_info::get_thread_id_main() == std::this_thread::get_id())
#define SFG_VERIFY_THREAD_RENDER()	 SFG_ASSERT(thread_info::get_thread_id_render() == std::this_thread::get_id())
#define SFG_VERIFY_INIT()			 SFG_ASSERT(thread_info::get_is_init())
#define IS_RENDER_THREAD()			 thread_info::get_thread_id_main() == std::this_thread::get_id()
#else
#define SFG_REGISTER_THREAD_MAIN()
#define SFG_REGISTER_THREAD_RENDER()
#define SFG_SET_INIT(IS_INIT)
#define SFG_VERIFY_THREAD_MAIN()
#define SFG_VERIFY_THREAD_RENDER()
#define SFG_VERIFY_INIT()
#endif

	class frame_info
	{
	public:
		static double get_main_thread_time_milli()
		{
			return s_main_thread_time_milli.load();
		}

		static double get_render_thread_time_milli()
		{
			return s_render_thread_time_milli.load();
		}

		static double get_render_thread_elapsed_seconds()
		{
			return s_render_thread_elapsed_seconds.load();
		}

		static u32 get_fps()
		{
			return s_fps.load();
		}

		static u64 get_frame()
		{
			return s_frame.load();
		}

		static u64 get_render_frame()
		{
			return s_render_frame.load();
		}

		static inline bool get_is_render_active()
		{
			return s_is_render_active;
		}

		static inline void add_draw_call(u32 ctx)
		{
			s_draw_calls += ctx;
		}

		static inline u64 get_draw_calls()
		{
			return s_draw_calls_ui.load(std::memory_order_acquire);
		}

	private:
		friend class app;
		friend class renderer;

	private:
		static inline void set_render_joined(bool b)
		{
			s_is_render_active = b;
		}

		static atomic<double> s_main_thread_time_milli;
		static atomic<double> s_render_thread_time_milli;
		static atomic<double> s_render_thread_elapsed_seconds;
		static atomic<u32>	  s_fps;
		static atomic<u64>	  s_frame;
		static atomic<u64>	  s_render_frame;
		static u32			  s_draw_calls;
		static atomic<u32>	  s_draw_calls_ui;
		static bool			  s_is_render_active;
	};

#define SFG_VERIFY_RENDER_NOT_RUNNING()					 SFG_ASSERT(!frame_info::get_is_render_active())
#define SFG_VERIFY_RENDER_THREAD()						 SFG_ASSERT(frame_info::get_is_render_active())
#define SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD() SFG_ASSERT(thread_info::get_thread_id_render() == std::this_thread::get_id() || !frame_info::get_is_render_active())
}
