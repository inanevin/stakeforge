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

#include "chunk_allocator.hpp"
#include "math/math_common.hpp"

#include "memory/memory_tracer.hpp"

namespace SFG
{

	chunk_allocator32::~chunk_allocator32()
	{
		SFG_ASSERT(_raw == nullptr);
		if (_raw != nullptr)
			uninit();
	}

	void chunk_allocator32::init(size_t size)
	{
		const size_t alignment = alignof(u32);
		SFG_ASSERT(size % alignment == 0);

		const size_t mem_size = ALIGN_UP(size, alignment);
		_raw				  = reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, mem_size));
		_total_size			  = mem_size;

#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer::get().on_allocation(_raw, mem_size);
#endif
	}

	void chunk_allocator32::uninit()
	{
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer::get().on_free(_raw);
#endif

		SFG_ASSERT(_raw != nullptr);
		SFG_ALIGNED_FREE(_raw);
		_raw = nullptr;
	}

	void chunk_allocator32::reset()
	{
		_free_chunks.resize(0);
		_head = 0;
	}

	chunk_handle32 chunk_allocator32::allocate_text(const string_t& source)
	{
		const size_t		 len	= source.size();
		const chunk_handle32 handle = allocate<u8>(len + 1);
		char*				 dst	= (char*)get<u8>(handle);
		SFG_MEMCPY(dst, source.data(), len);
		dst[len] = '\0';
		return handle;
	}
}
