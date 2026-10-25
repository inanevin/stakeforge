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

#include "core/memory/memory.hpp"
#include "core/memory/memory_tracer.hpp"

namespace sfg
{
	void* malloc_traced(size_t size)
	{
		void* ptr = malloc(size);
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_allocation(ptr, size);
#endif
		return ptr;
	}

	void free_traced(void* ptr)
	{
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_free(ptr);
#endif
		free(ptr);
	}

	void* aligned_malloc_traced(size_t alignment, size_t size)
	{
#ifdef SFG_COMPILER_MSVC
		void* ptr = _aligned_malloc(size, alignment);
#else
		void* ptr = std::aligned_alloc(alignment, size);
#endif
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_allocation(ptr, size);
#endif
		return ptr;
	}

	void aligned_free_traced(void* ptr)
	{
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_free(ptr);
#endif
#ifdef SFG_COMPILER_MSVC
		_aligned_free(ptr);
#else
		std::free(ptr);
#endif
	}
}
