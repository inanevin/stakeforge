/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

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
		static void update_render_thread(i64 time_us, i64 wait_time_us);

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
