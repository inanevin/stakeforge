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
