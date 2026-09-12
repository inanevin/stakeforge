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

#include "perf_metrics.hpp"

namespace sfg
{
	atomic_t<i64> perf_metrics_t::s_main_thread_time_us		   = 0;
	atomic_t<i64> perf_metrics_t::s_render_thread_time_us	   = 0;
	atomic_t<i64> perf_metrics_t::s_render_thread_work_time_us = 0;
	atomic_t<f32> perf_metrics_t::s_main_thread_fps			   = 0.0f;
	atomic_t<f32> perf_metrics_t::s_render_thread_fps		   = 0.0f;

	void perf_metrics_t::update_main_thread(i64 time_us)
	{
		s_main_thread_time_us.store(time_us, std::memory_order_relaxed);
		s_main_thread_fps.store(time_us == 0 ? 0.0f : 1000000.0f / static_cast<f32>(time_us), std::memory_order_relaxed);
	}

	void perf_metrics_t::update_render_thread(i64 time_us, i64 wait_time_us)
	{
		s_render_thread_time_us.store(time_us, std::memory_order_relaxed);
		s_render_thread_work_time_us.store(time_us - wait_time_us, std::memory_order_relaxed);
		s_render_thread_fps.store(time_us == 0 ? 0.0f : 1000000.0f / static_cast<f32>(time_us), std::memory_order_relaxed);
	}
}
