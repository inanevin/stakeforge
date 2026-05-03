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

#include "frame_allocator.hpp"
#include "memory.hpp"
#include "memory_tracer.hpp"
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
		SFG_MEMTRACE_ALLOC(raw, allocated_size);
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
			SFG_MEMTRACE_DEALLOC(g_frame_allocator_state._raw);
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
