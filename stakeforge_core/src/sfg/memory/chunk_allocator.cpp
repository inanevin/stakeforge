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

#include "chunk_allocator.hpp"
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
		SFG_ASSERT(mem_size <= UINT32_MAX);

		_raw		= reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignment, mem_size));
		_total_size = static_cast<u32>(mem_size);
		SFG_ASSERT(_raw != nullptr);
	}

	void chunk_allocator_t::uninit()
	{
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

	chunk_handle32_t chunk_allocator_t::allocate_bytes(size_t size, size_t alignment)
	{
		SFG_ASSERT(size != 0);
		SFG_ASSERT(alignment != 0);
		SFG_ASSERT((alignment & (alignment - 1)) == 0);
		SFG_ASSERT(alignment <= alignof(std::max_align_t));
		SFG_ASSERT(size <= UINT32_MAX);
		SFG_ASSERT(static_cast<size_t>(_head) + size < UINT32_MAX);

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

		SFG_ASSERT(_head <= _total_size);
		SFG_ASSERT(needed_size <= _total_size - _head);

		const chunk_handle32_t ret{current_aligned_head, requested_size};
		_head += needed_size;
		return ret;
	}

	chunk_handle32_t chunk_allocator_t::allocate_text(const char* src)
	{
		const size_t		   len	  = strlen(src);
		const chunk_handle32_t handle = allocate<u8>(len + 1);
		char*				   dst	  = (char*)get<u8>(handle);
		SFG_MEMCPY(dst, src, len);
		dst[len] = '\0';
		return handle;
	}

	const char* chunk_allocator_t::get_text(chunk_handle32_t handle)
	{
		if (handle.size == 0)
			return nullptr;

		const u8* data = _raw + handle.head;
		return reinterpret_cast<const char*>(data);
	}

}
