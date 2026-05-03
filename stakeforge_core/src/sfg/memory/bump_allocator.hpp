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
