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

#include "common/size_definitions.hpp"
#include "io/assert.hpp"
#include "memory.hpp"

namespace SFG
{
	template <typename T, int N> struct static_array
	{
		~static_array()
		{
		}

		static_array()
		{
		}

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------
		inline void reset()
		{
			for (u32 i = 0; i < N; i++)
				_items[i] = T();
		}

		inline void reset(u32 idx)
		{
			SFG_ASSERT(idx < N);
			_items[idx] = T();
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		T& get(u32 idx)
		{
			SFG_ASSERT(idx < N);
			return _items[idx];
		}

		const T& get(u32 idx) const
		{
			SFG_ASSERT(idx < N);
			return _items[idx];
		}

		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		template <typename TYPE> struct iterator
		{
			using reference = TYPE&;
			using pointer	= TYPE*;

			iterator(pointer ptr, u32 begin, u32 end) : _ptr(ptr), _current(begin), _end(end)
			{
			}

			reference operator*() const
			{
				return *(_ptr + _current);
			};
			pointer operator->()
			{
				return _ptr + _current;
			}

			iterator& operator++()
			{
				_current++;
				return *this;
			}

			iterator& operator++(int)
			{
				iterator tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const iterator& a, const iterator& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const iterator& a, const iterator& b)
			{
				return a._current != b._current;
			}

			pointer _ptr	 = nullptr;
			u32		_current = 0;
			u32		_end	 = 0;
		};

		iterator<const T> begin() const
		{
			return iterator<const T>(_items, 0, N);
		}

		iterator<const T> end() const
		{
			return iterator<const T>(_items, N, N);
		}

		iterator<T> begin()
		{
			return iterator<T>(_items, 0, N);
		}

		iterator<T> end()
		{
			return iterator<T>(_items, N, N);
		}

	private:
		T _items[N];
	};

}
