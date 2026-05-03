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
#include "memory_tracer.hpp"
#include <sfg/math/math_common.hpp>

namespace sfg
{

	chunk_allocator_t::~chunk_allocator_t()
	{
		SFG_ASSERT(_raw == nullptr);
		if (_raw != nullptr)
			uninit();
	}

	void chunk_allocator_t::init(size_t size)
	{
		SFG_ASSERT(_raw == nullptr);
		SFG_ASSERT(size != 0);

		const size_t alignment = alignof(std::max_align_t);
		const size_t mem_size  = ALIGN_UP(size, alignment);
		SFG_ASSERT(mem_size <= std::numeric_limits<u32>::max());

		_raw		= reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, mem_size));
		_total_size = static_cast<u32>(mem_size);
		SFG_ASSERT(_raw != nullptr);

#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_allocation(_raw, mem_size);
#endif
	}

	void chunk_allocator_t::uninit()
	{
#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_free(_raw);
#endif

		SFG_ASSERT(_raw != nullptr);
		SFG_ALIGNED_FREE(_raw);
		_raw		= nullptr;
		_head		= 0;
		_total_size = 0;
		_free_chunks.resize(0);
	}

	void chunk_allocator_t::reset()
	{
		_free_chunks.resize(0);
		_head = 0;
	}

	void chunk_allocator_t::reserve(size_t total_size)
	{
		if (total_size <= _total_size)
			return;

		grow_to(total_size);
	}

	chunk_handle32_t chunk_allocator_t::allocate_bytes(size_t size, size_t alignment)
	{
		SFG_ASSERT(size != 0);
		SFG_ASSERT(alignment != 0);
		SFG_ASSERT((alignment & (alignment - 1)) == 0);
		SFG_ASSERT(alignment <= alignof(std::max_align_t));
		SFG_ASSERT(size <= std::numeric_limits<u32>::max());

		const u32 requested_size = static_cast<u32>(size);

		if (!_free_chunks.empty())
		{
			for (auto it = _free_chunks.begin(); it != _free_chunks.end(); ++it)
			{
				const chunk_handle32_t chunk = *it;

				const u32 aligned_head		= ALIGN_UP(chunk.head, static_cast<u32>(alignment));
				const u32 aligned_size_need = (aligned_head - chunk.head) + requested_size;

				if (chunk.size >= aligned_size_need)
				{
					_free_chunks.erase(it);

					const chunk_handle32_t allocated_chunk{aligned_head, requested_size};

					if (aligned_head > chunk.head)
						insert_free_chunk_sorted({chunk.head, aligned_head - chunk.head});

					const u32 remaining_size = chunk.size - aligned_size_need;
					if (remaining_size > 0)
						insert_free_chunk_sorted({allocated_chunk.head + allocated_chunk.size, remaining_size});

					return allocated_chunk;
				}
			}
		}

		const u32 current_aligned_head = ALIGN_UP(_head, static_cast<u32>(alignment));
		const u32 needed_size		   = (current_aligned_head - _head) + requested_size;

		if (_head > _total_size || needed_size > _total_size - _head)
			grow_to(static_cast<size_t>(current_aligned_head) + requested_size);

		const chunk_handle32_t ret{current_aligned_head, requested_size};
		_head += needed_size;
		return ret;
	}

	chunk_handle32_t chunk_allocator_t::allocate_text(const string_t& source)
	{
		const size_t		   len	  = source.size();
		const chunk_handle32_t handle = allocate<u8>(len + 1);
		char*				   dst	  = (char*)get<u8>(handle);
		SFG_MEMCPY(dst, source.data(), len);
		dst[len] = '\0';
		return handle;
	}

	void chunk_allocator_t::grow_to(size_t required_total_size)
	{
		const size_t alignment = alignof(std::max_align_t);
		size_t		 new_size  = _total_size == 0 ? alignment : _total_size;

		while (new_size < required_total_size)
			new_size *= 2;

		new_size = ALIGN_UP(new_size, alignment);
		SFG_ASSERT(new_size <= std::numeric_limits<u32>::max());

		u8* new_raw = reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, new_size));
		SFG_ASSERT(new_raw != nullptr);

		if (_raw != nullptr && _head != 0)
			SFG_MEMCPY(new_raw, _raw, _head);

#ifdef SFG_ENABLE_MEMORY_TRACER
		memory_tracer_t::get().on_allocation(new_raw, new_size);
		if (_raw != nullptr)
			memory_tracer_t::get().on_free(_raw);
#endif

		if (_raw != nullptr)
			SFG_ALIGNED_FREE(_raw);

		_raw		= new_raw;
		_total_size = static_cast<u32>(new_size);
	}
}
