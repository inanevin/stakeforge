// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>

namespace sfg
{
	struct engine_runtime_stats_t
	{
		atomic_t<double> main_thread_time_ms	  = 0.0;
		atomic_t<double> render_thread_time_ms	  = 0.0;
		atomic_t<u32>	 fps_main				  = 0;
		atomic_t<u32>	 fps_render				  = 0;
		atomic_t<u64>	 render_frame_counter	  = 0;
		atomic_t<double> app_elapsed_time_seconds = 0.0;
		atomic_t<u64>	 main_thread_id			  = 0;
		atomic_t<u64>	 render_thread_id		  = 0;

		inline void reset()
		{
			main_thread_time_ms		 = 0.0;
			render_thread_time_ms	 = 0.0;
			fps_main				 = 0;
			fps_render				 = 0;
			render_frame_counter	 = 0;
			app_elapsed_time_seconds = 0.0;
			main_thread_id			 = 0;
			render_thread_id		 = 0;
		}
	};

	extern engine_runtime_stats_t g_engine_runtime_stats;
}
