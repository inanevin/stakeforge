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

#include "core/runtime/engine/perf_metrics.hpp"

namespace sfg
{
	atomic_t<u32> perf_metrics_t::s_world_draw_call_count			= 0;
	atomic_t<u32> perf_metrics_t::s_world_visible_renderables_count = 0;
	atomic_t<u32> perf_metrics_t::s_world_bone_count				= 0;
	atomic_t<u32> perf_metrics_t::s_world_light_count				= 0;
	atomic_t<u32> perf_metrics_t::s_world_probe_count				= 0;
	atomic_t<u64> perf_metrics_t::s_ram_usage_bytes					= 0;
	atomic_t<u64> perf_metrics_t::s_vram_usage_bytes				= 0;

	atomic_t<i64> perf_metrics_t::s_main_thread_time_us		   = 0;
	atomic_t<i64> perf_metrics_t::s_render_thread_time_us	   = 0;
	atomic_t<i64> perf_metrics_t::s_render_thread_work_time_us = 0;
	atomic_t<f32> perf_metrics_t::s_main_thread_fps			   = 0.0f;
	atomic_t<f32> perf_metrics_t::s_render_thread_fps		   = 0.0f;

	void perf_metrics_t::update_world_draw_call_count(u32 count)
	{
		s_world_draw_call_count.store(count, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_world_visible_renderables_count(u32 count)
	{
		s_world_visible_renderables_count.store(count, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_world_bone_count(u32 count)
	{
		s_world_bone_count.store(count, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_world_light_count(u32 count)
	{
		s_world_light_count.store(count, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_world_probe_count(u32 count)
	{
		s_world_probe_count.store(count, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_ram_usage_bytes(u64 bytes)
	{
		s_ram_usage_bytes.store(bytes, std::memory_order_relaxed);
	}

	void perf_metrics_t::update_vram_usage_bytes(u64 bytes)
	{
		s_vram_usage_bytes.store(bytes, std::memory_order_relaxed);
	}

	void perf_metrics_t::add_world_draw_calls(u32 count)
	{
		s_world_draw_call_count.fetch_add(count, std::memory_order_relaxed);
	}

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
