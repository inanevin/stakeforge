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

#include "frame_allocator.hpp"
#include "memory.hpp"
#include <sfg/math/math_common.hpp>

namespace sfg
{
	thread_local frame_allocator_tls_t::frame_allocator_state_t g_frame_allocator_state;

	void frame_allocator_tls_t::init(size_t size, size_t alignment)
	{
		SFG_ASSERT(size != 0);
		SFG_ASSERT(IS_POW(alignment));
		SFG_ASSERT(g_frame_allocator_state._raw == nullptr);

		const size_t allocated_size = ALIGN_UP(size, alignment);
		u8*			 raw			= static_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, allocated_size));
		if (raw == nullptr)
			throw std::bad_alloc();

		g_frame_allocator_state._capacity		= size;
		g_frame_allocator_state._allocated_size = allocated_size;
		g_frame_allocator_state._head			= 0;
		g_frame_allocator_state._raw			= raw;
		g_frame_allocator_state._owns			= 1;
	}

	void frame_allocator_tls_t::init(u8* existing, size_t size)
	{
		SFG_ASSERT(existing != nullptr);
		SFG_ASSERT(size != 0);
		SFG_ASSERT(g_frame_allocator_state._raw == nullptr);

		g_frame_allocator_state._capacity		= size;
		g_frame_allocator_state._allocated_size = 0;
		g_frame_allocator_state._head			= 0;
		g_frame_allocator_state._raw			= existing;
		g_frame_allocator_state._owns			= 0;
	}

	void frame_allocator_tls_t::uninit()
	{
		if (g_frame_allocator_state._owns)
		{
			SFG_ALIGNED_FREE(g_frame_allocator_state._raw);
		}

		g_frame_allocator_state = {};
	}

	void frame_allocator_tls_t::reset()
	{
		SFG_ASSERT(g_frame_allocator_state._raw != nullptr);
		g_frame_allocator_state._head = 0;
	}

	void* frame_allocator_tls_t::allocate(size_t size, size_t alignment)
	{
		SFG_ASSERT(g_frame_allocator_state._raw != nullptr);
		SFG_ASSERT(IS_POW(alignment));

		if (g_frame_allocator_state._raw == nullptr)
			throw std::bad_alloc();

		void*  current_ptr = g_frame_allocator_state._raw + g_frame_allocator_state._head;
		size_t space	   = g_frame_allocator_state._capacity - g_frame_allocator_state._head;

		void* aligned_ptr = std::align(alignment, size, current_ptr, space);
		if (aligned_ptr == nullptr || size > space)
		{
			SFG_ASSERT(false);
			throw std::bad_alloc();
		}

		g_frame_allocator_state._head = g_frame_allocator_state._capacity - space + size;
		return aligned_ptr;
	}

	bool frame_allocator_tls_t::is_init()
	{
		return g_frame_allocator_state._raw != nullptr;
	}

	size_t frame_allocator_tls_t::get_capacity()
	{
		return g_frame_allocator_state._capacity;
	}

	size_t frame_allocator_tls_t::get_head()
	{
		return g_frame_allocator_state._head;
	}
}
