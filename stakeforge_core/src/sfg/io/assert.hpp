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

#include <cstdio>
#include <cstdlib>

namespace sfg
{

#ifdef SFG_PLATFORM_WINDOWS
#define DBG_BRK __debugbreak();
#else
#define DBG_BRK __builtin_trap();
#endif

#ifdef SFG_DEBUG
#define SFG_ASSERT(x, ...)                                                                                                                                                                                                                                         \
	if (!(x))                                                                                                                                                                                                                                                      \
	{                                                                                                                                                                                                                                                              \
		DBG_BRK                                                                                                                                                                                                                                                    \
	}

#else
#define SFG_ASSERT(x, ...)
#endif

#define SFG_FAIL(condition, message)                                                                                                                                                                                                                               \
	do                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                              \
		if (!(condition))                                                                                                                                                                                                                                          \
		{                                                                                                                                                                                                                                                          \
			SFG_ASSERT(condition, message);                                                                                                                                                                                                                        \
			std::fprintf(stderr, "Fatal: %s\nFile: %s:%d\n", (message), __FILE__, __LINE__);                                                                                                                                                                       \
			std::fflush(stderr);                                                                                                                                                                                                                                   \
			std::exit(EXIT_FAILURE);                                                                                                                                                                                                                               \
		}                                                                                                                                                                                                                                                          \
	} while (0)

#define SFG_NOTIMPLEMENTED static_assert(false, "Implementation missing!")

}
