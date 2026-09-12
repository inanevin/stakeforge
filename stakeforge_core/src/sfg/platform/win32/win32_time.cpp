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

#include <sfg/platform/time.hpp>
#include <sfg/io/log.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>

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
