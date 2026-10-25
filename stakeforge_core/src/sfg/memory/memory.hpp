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

#include <sfg/common/sfg_exports.hpp>

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
	SFG_API void* malloc_traced(size_t size);
	SFG_API void  free_traced(void* ptr);
	SFG_API void* aligned_malloc_traced(size_t alignment, size_t size);
	SFG_API void  aligned_free_traced(void* ptr);

	template <typename T, typename... Args> T* sfg_new(Args&&... args)
	{
		static_assert(!std::is_array_v<T>);

		T* const object = static_cast<T*>(aligned_malloc_traced(alignof(T), sizeof(T)));

		if (object == nullptr)
			throw std::bad_alloc{};

		try
		{
			return std::construct_at(object, std::forward<Args>(args)...);
		}
		catch (...)
		{
			aligned_free_traced(object);

			throw;
		}
	}

	template <typename T> void sfg_delete(T* object)
	{
		static_assert(!std::is_array_v<T>);

		if (object == nullptr)
			return;

		std::destroy_at(object);
		aligned_free_traced(object);
	}
}

#define SFG_MALLOC(SIZE)					sfg::malloc_traced(SIZE)
#define SFG_FREE(PTR)						sfg::free_traced(PTR)
#define SFG_ALIGNED_MALLOC(ALIGNMENT, SIZE) sfg::aligned_malloc_traced(ALIGNMENT, SIZE)
#define SFG_ALIGNED_FREE(PTR)				sfg::aligned_free_traced(PTR)
