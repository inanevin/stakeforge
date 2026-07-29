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
#include <sfg/data/atomic.hpp>

namespace sfg
{
	class perf_metrics_t final
	{
	public:
		static void update_main_thread(i64 time_us);
		static void update_render_thread(i64 time_us, i64 present_time_us);

		static inline i64 get_main_thread_time_us()
		{
			return s_main_thread_time_us.load(std::memory_order_relaxed);
		}

		static inline i64 get_render_thread_time_us()
		{
			return s_render_thread_time_us.load(std::memory_order_relaxed);
		}

		static inline i64 get_render_thread_work_time_us()
		{
			return s_render_thread_work_time_us.load(std::memory_order_relaxed);
		}

		static inline f32 get_main_thread_time_ms()
		{
			return static_cast<f32>(get_main_thread_time_us()) * 0.001f;
		}

		static inline f32 get_render_thread_time_ms()
		{
			return static_cast<f32>(get_render_thread_time_us()) * 0.001f;
		}

		static inline f32 get_render_thread_work_time_ms()
		{
			return static_cast<f32>(get_render_thread_work_time_us()) * 0.001f;
		}

		static inline f32 get_main_thread_fps()
		{
			return s_main_thread_fps.load(std::memory_order_relaxed);
		}

		static inline f32 get_render_thread_fps()
		{
			return s_render_thread_fps.load(std::memory_order_relaxed);
		}

	private:
		static atomic_t<i64> s_main_thread_time_us;
		static atomic_t<i64> s_render_thread_time_us;
		static atomic_t<i64> s_render_thread_work_time_us;
		static atomic_t<f32> s_main_thread_fps;
		static atomic_t<f32> s_render_thread_fps;
	};
}
