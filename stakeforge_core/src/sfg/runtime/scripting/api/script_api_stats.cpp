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

#include "script_api_stats.hpp"

#include <sfg/runtime/engine/perf_metrics.hpp>

namespace sfg
{
	f32 api_stats_get_main_thread_time_ms()
	{
		return perf_metrics_t::get_main_thread_time_ms();
	}

	f32 api_stats_get_main_thread_fps()
	{
		return perf_metrics_t::get_main_thread_fps();
	}

	f32 api_stats_get_render_work_time_ms()
	{
		return perf_metrics_t::get_render_thread_work_time_ms();
	}

	f32 api_stats_get_render_thread_time_ms()
	{
		return perf_metrics_t::get_render_thread_time_ms();
	}

	f32 api_stats_get_render_thread_fps()
	{
		return perf_metrics_t::get_render_thread_fps();
	}

	const script_api_stats_t& get_script_api_stats()
	{
		static const script_api_stats_t api{
			.size					   = static_cast<u32>(sizeof(script_api_stats_t)),
			.version				   = 1,
			.get_main_thread_time_ms   = api_stats_get_main_thread_time_ms,
			.get_main_thread_fps	   = api_stats_get_main_thread_fps,
			.get_render_work_time_ms   = api_stats_get_render_work_time_ms,
			.get_render_thread_time_ms = api_stats_get_render_thread_time_ms,
			.get_render_thread_fps	   = api_stats_get_render_thread_fps,
		};

		return api;
	}
}
