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

#include <stdlib.h>
#include <new>
#include <limits>

#undef max

namespace sfg
{
	template <class T> struct malloc_allocator_map_t
	{
		typedef size_t	  size_type;
		typedef ptrdiff_t difference_type;
		typedef T*		  pointer;
		typedef const T*  const_pointer;
		typedef T&		  reference;
		typedef const T&  const_reference;
		typedef T		  value_type;

		template <class U> struct rebind_t
		{
			typedef malloc_allocator_map_t<U> other;
		};
		malloc_allocator_map_t() throw()
		{
		}
		malloc_allocator_map_t(const malloc_allocator_map_t&) throw()
		{
		}

		template <class U> malloc_allocator_map_t(const malloc_allocator_map_t<U>&) throw()
		{
		}

		~malloc_allocator_map_t() throw()
		{
		}

		pointer address(reference x) const
		{
			return &x;
		}
		const_pointer address(const_reference x) const
		{
			return &x;
		}

		pointer allocate(size_type s, void const* = 0)
		{
			if (0 == s)
				return NULL;
			pointer temp = (pointer)malloc(s * sizeof(T));
			if (temp == NULL)
				throw std::bad_alloc();
			return temp;
		}

		void deallocate(pointer p, size_type)
		{
			free(p);
		}

		size_type max_size() const throw()
		{
			return std::numeric_limits<size_t>::max() / sizeof(T);
		}

		void construct(pointer p, const T& val)
		{
			new ((void*)p) T(val);
		}

		void destroy(pointer p)
		{
			p->~T();
		}
	};

}
