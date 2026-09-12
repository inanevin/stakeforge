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

#include "memory.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/io/assert.hpp>
#include <type_traits>

namespace sfg
{
	class bump_allocator_t
	{
	public:
		void init(size_t sz, size_t alignment);
		void init(u8* existing, size_t sz);
		void uninit();

		bump_allocator_t()										   = default;
		bump_allocator_t& operator=(const bump_allocator_t& other) = delete;
		bump_allocator_t(const bump_allocator_t& other)			   = delete;
		~bump_allocator_t()
		{
			SFG_ASSERT(!_owns || _raw == nullptr);
		}

		void* allocate(size_t size, size_t alignment = 1);

		inline void reset()
		{
			_head = 0;
		}

		template <typename T, typename... Args> T* allocate(size_t count, Args&&... args)
		{
			static_assert(std::is_trivially_copyable_v<T>, "bump_allocator_t typed allocation only supports trivially copyable types");
			static_assert(std::is_trivially_destructible_v<T>, "bump_allocator_t typed allocation only supports trivially destructible types");
			static_assert(sizeof...(Args) == 0, "bump_allocator_t typed allocation does not construct objects; use only trivial raw allocation");

			if (count == 0)
				return nullptr;

			void* ptr	   = allocate(sizeof(T) * count, std::alignment_of<T>::value);
			T*	  arrayPtr = reinterpret_cast<T*>(ptr);
			return arrayPtr;
		}

		template <typename T, typename... Args> T* emplace_aux(T firstValue, Args&&... remainingValues)
		{
			static_assert(std::is_trivially_copyable_v<T>, "bump_allocator_t::emplace_aux only supports trivially copyable types");

			u8* initial_head = (u8*)_raw + _head;

			u8* current_head = initial_head;
			SFG_MEMCPY(current_head, &firstValue, sizeof(T));
			_head += sizeof(T);
			SFG_ASSERT(_head < _size);

			if constexpr (sizeof...(remainingValues) > 0)
			{
				emplace_aux<T>(remainingValues...);
			}

			return reinterpret_cast<T*>(initial_head);
		}

		inline size_t get_size() const
		{
			return _size;
		}

		inline size_t get_head() const
		{
			return _head;
		}

	private:
		size_t _size = 0;
		size_t _head = 0;
		void*  _raw	 = nullptr;
		u8	   _owns = 0;
	};
}
