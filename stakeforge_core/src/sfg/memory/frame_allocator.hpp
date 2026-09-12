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
			{
				SFG_ASSERT(false);
				return nullptr;
			}

			if (n == 0)
			{
				SFG_ASSERT(false);
				return nullptr;
			}

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

}
