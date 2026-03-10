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
#include "chunk_handle.hpp"
#include "io/assert.hpp"
#include "data/vector.hpp"
#include "data/vector_util.hpp"
#include "data/string.hpp"
#include "math/math_common.hpp"
#include "memory.hpp"

namespace SFG
{
	class chunk_allocator32
	{
	public:
		~chunk_allocator32();

		void		   init(size_t total_size);
		void		   uninit();
		void		   reset();
		chunk_handle32 allocate_text(const string& source);

		template <typename T> inline chunk_handle32 allocate(size_t count)
		{
			SFG_ASSERT(count != 0);

			const size_t item_alignment	  = alignof(T);
			const size_t padded_item_size = ALIGN_UP(sizeof(T), item_alignment);
			const u32	 requested_size	  = static_cast<u32>(padded_item_size * count);

			// Try reuse from free list (always sorted)
			if (!_free_chunks.empty())
			{
				for (auto it = _free_chunks.begin(); it != _free_chunks.end(); ++it)
				{
					const chunk_handle32 chunk = *it;

					const u32 aligned_head		= ALIGN_UP(chunk.head, item_alignment);
					const u32 aligned_size_need = (aligned_head - chunk.head) + requested_size;

					if (chunk.size >= aligned_size_need)
					{
						_free_chunks.erase(it);

						const chunk_handle32 allocated_chunk{aligned_head, requested_size};

						// pre-split
						if (aligned_head > chunk.head)
							insert_free_chunk_sorted({chunk.head, aligned_head - chunk.head});

						// post-split
						const u32 remaining_size = chunk.size - aligned_size_need;
						if (remaining_size > 0)
							insert_free_chunk_sorted({allocated_chunk.head + allocated_chunk.size, remaining_size});

						// Default-initialize payload (see note below)
						T* ptr = reinterpret_cast<T*>(_raw + allocated_chunk.head);
						for (size_t i = 0; i < count; ++i)
							ptr[i] = T();

						return allocated_chunk;
					}
				}
			}

			// Fallback: bump the head
			const u32 current_aligned_head = ALIGN_UP(_head, item_alignment);
			const u32 needed_size		   = (current_aligned_head - _head) + requested_size;

			SFG_ASSERT(_head <= _total_size && needed_size <= _total_size - _head);

			const chunk_handle32 ret{current_aligned_head, requested_size};

			T* ptr = reinterpret_cast<T*>(_raw + ret.head);
			for (size_t i = 0; i < count; ++i)
				ptr[i] = T();

			_head += needed_size;
			return ret;
		}

		template <typename T> inline chunk_handle32 allocate(size_t count, T*& out)
		{
			const chunk_handle32 ret = allocate<T>(count);
			out						 = get<T>(ret);
			return ret;
		}

		inline void free(chunk_handle32 handle)
		{
			SFG_ASSERT(handle.size != 0);
			SFG_MEMSET(_raw + handle.head, 0, handle.size); // optional
			insert_free_chunk_sorted(handle);
		}

		template <typename T> T* get(chunk_handle32 handle)
		{
			SFG_ASSERT(handle.size != 0);
			return reinterpret_cast<T*>(_raw + handle.head);
		}

		template <typename T> T* get(chunk_handle32 handle) const
		{
			SFG_ASSERT(handle.size != 0);
			return reinterpret_cast<T*>(_raw + handle.head);
		}

		inline u8* get(u32 index)
		{
			return _raw + index;
		}
		inline u32 get_current() const
		{
			return _head;
		}

	private:
		// Insert while keeping order and coalescing neighbors.
		inline void insert_free_chunk_sorted(chunk_handle32 c)
		{
			auto it = std::lower_bound(_free_chunks.begin(), _free_chunks.end(), c, [](const chunk_handle32& a, const chunk_handle32& b) { return a.head < b.head; });

			it = _free_chunks.insert(it, c); // insert c at sorted position

			// Merge with previous if adjacent
			if (it != _free_chunks.begin())
			{
				auto prev = it - 1;
				if (prev->head + prev->size == it->head)
				{
					prev->size += it->size;
					it = _free_chunks.erase(it); // drop current, keep prev
					it = prev;					 // iterator now at merged block
				}
			}

			// Merge with next if adjacent
			if (it + 1 != _free_chunks.end())
			{
				auto next = it + 1;
				if (it->head + it->size == next->head)
				{
					it->size += next->size;
					_free_chunks.erase(next);
				}
			}
		}

	private:
		u8*					   _raw = nullptr;
		vector<chunk_handle32> _free_chunks; // ALWAYS kept sorted by head
		u32					   _head	   = 0;
		u32					   _total_size = 0;
	};

}
