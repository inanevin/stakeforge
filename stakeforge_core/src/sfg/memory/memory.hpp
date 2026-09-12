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

#ifdef SFG_COMPILER_MSVC
#include <malloc.h>
#else
#include <cstdlib>
#endif

#include <memory>
#define SFG_MEMCPY(...)	 memcpy(__VA_ARGS__)
#define SFG_MEMMOVE(...) memmove(__VA_ARGS__)
#define SFG_MEMSET(...)	 memset(__VA_ARGS__)
#define SFG_MEMCMP(...)	 memcmp(__VA_ARGS__)

namespace sfg
{
	void* malloc_traced(size_t size);
	void  free_traced(void* ptr);
	void* aligned_malloc_traced(size_t alignment, size_t size);
	void  aligned_free_traced(void* ptr);
}

#define SFG_MALLOC(SIZE)					sfg::malloc_traced(SIZE)
#define SFG_FREE(PTR)						sfg::free_traced(PTR)
#define SFG_ALIGNED_MALLOC(ALIGNMENT, SIZE) sfg::aligned_malloc_traced(ALIGNMENT, SIZE)
#define SFG_ALIGNED_FREE(PTR)				sfg::aligned_free_traced(PTR)
