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
