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

#include "bump_allocator.hpp"
#include "memory.hpp"
#include "memory_tracer.hpp"

namespace sfg
{
	void bump_allocator_t::init(size_t sz, size_t alignment)
	{
		SFG_ASSERT(sz != 0);
		_size = sz;
		_raw  = SFG_ALIGNED_MALLOC(alignment, sz);
		_owns = 1;
		SFG_MEMTRACE_ALLOC(_raw, sz);
	}

	void bump_allocator_t::init(u8* existing, size_t sz)
	{
		_owns = 0;
		_raw  = existing;
		_size = sz;
	}

	void bump_allocator_t::uninit()
	{
		if (_owns)
		{
			SFG_MEMTRACE_DEALLOC(_raw);
			SFG_ALIGNED_FREE(_raw);
		}
		_raw = nullptr;
	}

	void* bump_allocator_t::allocate(size_t size, size_t alignment)
	{
		SFG_ASSERT(size <= _size - _head);

		void*  current_ptr = (void*)((u8*)_raw + _head);
		size_t space	   = _size - _head;

		void* aligned_ptr = std::align(alignment, size, current_ptr, space);
		if (aligned_ptr == nullptr)
			return nullptr;

		_head = _size - space + size;
		return aligned_ptr;
	}
}
