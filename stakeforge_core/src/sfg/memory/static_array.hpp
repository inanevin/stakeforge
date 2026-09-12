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
#include <sfg/io/assert.hpp>
#include "memory.hpp"

namespace sfg
{
	template <typename T, int N> struct static_array_t
	{
		~static_array_t()
		{
		}

		static_array_t()
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

		template <typename TYPE> struct iterator_t
		{
			using reference = TYPE&;
			using pointer	= TYPE*;

			iterator_t(pointer ptr, u32 begin, u32 end) : _ptr(ptr), _current(begin), _end(end)
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

			iterator_t& operator++()
			{
				_current++;
				return *this;
			}

			iterator_t& operator++(int)
			{
				iterator_t tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const iterator_t& a, const iterator_t& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const iterator_t& a, const iterator_t& b)
			{
				return a._current != b._current;
			}

			pointer _ptr	 = nullptr;
			u32		_current = 0;
			u32		_end	 = 0;
		};

		iterator_t<const T> begin() const
		{
			return iterator_t<const T>(_items, 0, N);
		}

		iterator_t<const T> end() const
		{
			return iterator_t<const T>(_items, N, N);
		}

		iterator_t<T> begin()
		{
			return iterator_t<T>(_items, 0, N);
		}

		iterator_t<T> end()
		{
			return iterator_t<T>(_items, N, N);
		}

	private:
		T _items[N];
	};

}
