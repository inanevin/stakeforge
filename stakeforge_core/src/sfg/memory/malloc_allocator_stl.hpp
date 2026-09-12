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

#undef min
#undef max

#include <stdlib.h>
#include <new>
#include <limits>

namespace sfg
{
	template <class T> struct malloc_allocator_stl_t
	{
		typedef T value_type;

		malloc_allocator_stl_t() = default;

		template <class U> constexpr malloc_allocator_stl_t(const malloc_allocator_stl_t<U>&) noexcept
		{
		}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
				throw std::bad_array_new_length();

			if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
			{
				return p;
			}

			throw std::bad_alloc();
		}

		void deallocate(T* p, std::size_t n) noexcept
		{
			std::free(p);
		}
	};

	template <class T, class U> bool operator==(const malloc_allocator_stl_t<T>&, const malloc_allocator_stl_t<U>&)
	{
		return true;
	}

	template <class T, class U> bool operator!=(const malloc_allocator_stl_t<T>&, const malloc_allocator_stl_t<U>&)
	{
		return false;
	}
}
