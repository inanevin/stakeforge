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

#include <sfg/common/size_definitions.hpp>

#ifdef SFG_PLATFORM_OSX
#include <mach/mach_time.h>
#endif

namespace sfg
{
	class time_t
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

		static inline double micro_to_ms(i64 microseconds)
		{
			return static_cast<double>(microseconds) * 0.001;
		}

		static inline double micro_to_s(i64 microseconds)
		{
			return static_cast<double>(microseconds) * 0.000001;
		}

	private:
#ifdef SFG_PLATFORM_OSX
		static mach_timebase_info_data_t s_timebaseInfo;
#endif

#ifdef SFG_PLATFORM_WINDOWS
		static i64 s_frequency;
#endif
	};

} // namespace sfg
