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

#include "platform/time.hpp"
#include "io/log.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib, "Winmm.lib")

namespace sfg
{
	i64 time_t::s_frequency = 0;

	void time_t::init()
	{
		if (s_frequency == 0)
		{
			timeBeginPeriod(1);

			LARGE_INTEGER frequency;

			if (!QueryPerformanceFrequency(&frequency))
			{
				SFG_ERR("[time] -> QueryPerformanceFrequency failed!");
			}

			s_frequency = frequency.QuadPart;
		}
	}

	void time_t::uninit()
	{
		timeEndPeriod(1);
	}

	i64 time_t::get_cpu_microseconds()
	{
		LARGE_INTEGER cycles;
		QueryPerformanceCounter(&cycles);

		// directly converting cycles to microseconds will overflow
		// first dividing with frequency will turn it into seconds and loose precision.
		return (cycles.QuadPart / s_frequency) * 1000000ll + ((cycles.QuadPart % s_frequency) * 1000000ll) / s_frequency;
	}

	double time_t::get_cpu_seconds()
	{
		LARGE_INTEGER cycles;
		QueryPerformanceCounter(&cycles);
		return static_cast<double>(cycles.QuadPart) * 1.0 / static_cast<double>(s_frequency);
	}

	i64 time_t::get_cpu_cycles()
	{
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
	}

	double time_t::get_delta_seconds(i64 fromCycles, i64 toCycles)
	{
		return static_cast<double>(toCycles - fromCycles) * 1.0 / (static_cast<double>(s_frequency));
	}

	i64 time_t::get_delta_microseconds(i64 fromCycles, i64 toCycles)
	{
		return ((toCycles - fromCycles) * 1000000ll) / s_frequency;
	}

	void time_t::throttle(i64 microseconds)
	{
		if (microseconds < 0)
			return;

		i64		  now	 = get_cpu_microseconds();
		const i64 target = now + microseconds;
		i64		  sleep	 = microseconds;

		for (;;)
		{
			now = get_cpu_microseconds();

			if (now >= target)
			{
				break;
			}

			i64 diff = target - now;

			if (diff > 2000)
			{
				u32 ms = static_cast<u32>((double)(diff - 2000) / 1000.0);
				go_to_sleep(ms);
			}
			else
			{
				go_to_sleep(0);
			}
		}
	}

	void time_t::go_to_sleep(u32 milliseconds)
	{
		if (milliseconds == 0)
			YieldProcessor();
		else
			::Sleep(milliseconds);
	}

	void time_t::yield_thread()
	{
		YieldProcessor();
	}

} // namespace sfg
