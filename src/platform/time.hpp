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

#ifdef SFG_PLATFORM_OSX
#include <mach/mach_time.h>
#endif

namespace SFG
{
	class time
	{
	public:
		static void	  init();
		static void	  uninit();
		static i64	  get_cpu_microseconds();
		static i64	  get_cpu_cycles();
		static double get_cpu_seconds();
		static double get_delta_seconds(i64 fromCycles, i64 toCycles);
		static i64	  get_delta_microseconds(i64 fromCycles, i64 toCycles);
		static void	  throttle(i64 microseconds);
		static void	  go_to_sleep(u32 milliseconds);
		static void	  yield_thread();

	private:
#ifdef SFG_PLATFORM_OSX
		static mach_timebase_info_data_t s_timebaseInfo;
#endif

#ifdef SFG_PLATFORM_WINDOWS
		static i64 s_frequency;
#endif
	};

} // namespace SFG
