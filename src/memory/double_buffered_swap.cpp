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

#include "double_buffered_swap.hpp"
#include "io/assert.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"

namespace SFG
{
	void double_buffered_swap::init(size_t sz, size_t alignment)
	{
		_data[0] = reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, sz));
		_data[1] = reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, sz));

#ifdef SFG_DEBUG
		_sz = static_cast<u32>(sz);
		PUSH_ALLOCATION_SZ(sz * 2);
#endif
	}

	void double_buffered_swap::uninit()
	{
		SFG_ALIGNED_FREE(_data[0]);
		SFG_ALIGNED_FREE(_data[1]);

#ifdef SFG_DEBUG
		PUSH_DEALLOCATION_SZ(_sz * 2);
#endif
	}

	void double_buffered_swap::write(const void* src, size_t padding, size_t n)
	{
		SFG_ASSERT(padding + n < _sz);

		uint8_t cur = _index.load(std::memory_order_relaxed);
		SFG_MEMCPY(_data[cur] + padding, src, n);
	}

	void double_buffered_swap::swap()
	{
		uint8_t cur	 = _index.load(std::memory_order_relaxed);
		uint8_t next = cur ^ 1;
		_index.store(next, std::memory_order_release);
	}

	void double_buffered_swap::read(void* dst, size_t n) const
	{
		SFG_ASSERT(n < _sz);

		uint8_t cur = _index.load(std::memory_order_acquire);
		std::memcpy(dst, _data[cur], n);
	}
}
