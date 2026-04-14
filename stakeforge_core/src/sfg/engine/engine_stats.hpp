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

#include "common/size_definitions.hpp"
#include "data/atomic.hpp"

namespace sfg
{
	struct engine_stats_t
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

	extern engine_stats_t g_engine_stats;
}
