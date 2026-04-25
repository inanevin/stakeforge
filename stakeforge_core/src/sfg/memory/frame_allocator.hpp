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

#include <cstddef>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

namespace sfg
{
	class frame_allocator_tls_t
	{
	public:
		struct frame_allocator_state_t
		{
			size_t _capacity	   = 0;
			size_t _allocated_size = 0;
			size_t _head		   = 0;
			u8*	   _raw			   = nullptr;
			u8	   _owns		   = 0;
		};

	public:
		static void init(size_t size, size_t alignment = alignof(std::max_align_t));
		static void init(u8* existing, size_t size);
		static void uninit();
		static void reset();

		static void* allocate(size_t size, size_t alignment);

		static bool	  is_init();
		static size_t get_capacity();
		static size_t get_head();
	};

	extern thread_local frame_allocator_tls_t::frame_allocator_state_t g_frame_allocator_state;

	template <class T> struct frame_allocator_t
	{
		typedef T value_type;

		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal						 = std::true_type;

		frame_allocator_t() = default;

		template <class U> constexpr frame_allocator_t(const frame_allocator_t<U>&) noexcept
		{
		}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
				throw std::bad_array_new_length();

			if (n == 0)
				return nullptr;

			return static_cast<T*>(frame_allocator_tls_t::allocate(n * sizeof(T), alignof(T)));
		}

		void deallocate(T*, std::size_t) noexcept
		{
		}
	};

	template <class T, class U> bool operator==(const frame_allocator_t<T>&, const frame_allocator_t<U>&)
	{
		return true;
	}

	template <class T, class U> bool operator!=(const frame_allocator_t<T>&, const frame_allocator_t<U>&)
	{
		return false;
	}

	template <typename T> using frame_vector_t = std::vector<T, frame_allocator_t<T>>;
	using frame_string_t					   = std::basic_string<char, std::char_traits<char>, frame_allocator_t<char>>;
}
