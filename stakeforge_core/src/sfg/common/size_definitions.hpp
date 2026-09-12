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

#ifdef SFG_PLATFORM_OSX
#include <stddef.h>
#endif
#include <stdint.h>

typedef signed char		   i8;
typedef short			   i16;
typedef int				   i32;
typedef long long		   i64;
typedef unsigned char	   u8;
typedef unsigned short	   u16;
typedef unsigned int	   u32;
typedef unsigned long long u64;
typedef float			   f32;

namespace sfg
{
	using sid_t = u64;
}

#define NULL_SID UINT64_MAX
