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

#include "system_info.hpp"

namespace SFG
{
#ifdef SFG_DEBUG
	std::thread::id thread_info::s_thread_id_render;
	std::thread::id thread_info::s_thread_id_main;
	bool			thread_info::s_is_init;
#endif

	atomic<double> frame_info::s_main_thread_time_milli		   = 0;
	atomic<double> frame_info::s_render_thread_time_milli	   = 0;
	atomic<double> frame_info::s_render_thread_elapsed_seconds = 0;
	atomic<u32>	   frame_info::s_fps						   = 0;
	atomic<u64>	   frame_info::s_frame						   = 0;
	atomic<u64>	   frame_info::s_render_frame				   = 0;
	u32			   frame_info::s_draw_calls					   = 0;
	atomic<u32>	   frame_info::s_draw_calls_ui				   = 0;
	bool		   frame_info::s_is_render_active			   = 0;
}
