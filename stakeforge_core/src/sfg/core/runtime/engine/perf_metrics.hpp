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

#include "core/common/sfg_exports.hpp"

#include "core/common/size_definitions.hpp"
#include "core/data/atomic.hpp"

namespace sfg
{
	class SFG_API perf_metrics_t final
	{
	public:
		static void update_world_draw_call_count(u32 count);
		static void update_world_visible_renderables_count(u32 count);
		static void update_world_bone_count(u32 count);
		static void update_world_light_count(u32 count);
		static void update_world_probe_count(u32 count);
		static void update_ram_usage_bytes(u64 bytes);
		static void update_vram_usage_bytes(u64 bytes);
		static void add_world_draw_calls(u32 count);

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

		static inline u32 get_world_draw_call_count()
		{
			return s_world_draw_call_count.load(std::memory_order_relaxed);
		}

		static inline u32 get_world_visible_renderables_count()
		{
			return s_world_visible_renderables_count.load(std::memory_order_relaxed);
		}

		static inline u32 get_world_bone_count()
		{
			return s_world_bone_count.load(std::memory_order_relaxed);
		}

		static inline u32 get_world_light_count()
		{
			return s_world_light_count.load(std::memory_order_relaxed);
		}

		static inline u32 get_world_probe_count()
		{
			return s_world_probe_count.load(std::memory_order_relaxed);
		}

		static inline u64 get_ram_usage_bytes()
		{
			return s_ram_usage_bytes.load(std::memory_order_relaxed);
		}

		static inline u64 get_vram_usage_bytes()
		{
			return s_vram_usage_bytes.load(std::memory_order_relaxed);
		}

	private:
		static atomic_t<u32> s_world_draw_call_count;
		static atomic_t<u32> s_world_visible_renderables_count;
		static atomic_t<u32> s_world_bone_count;
		static atomic_t<u32> s_world_light_count;
		static atomic_t<u32> s_world_probe_count;
		static atomic_t<u64> s_ram_usage_bytes;
		static atomic_t<u64> s_vram_usage_bytes;
		static atomic_t<i64> s_main_thread_time_us;
		static atomic_t<i64> s_render_thread_time_us;
		static atomic_t<i64> s_render_thread_work_time_us;
		static atomic_t<f32> s_main_thread_fps;
		static atomic_t<f32> s_render_thread_fps;
	};
}
