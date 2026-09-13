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

#include "chunk_handle.hpp"
#include "memory.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/data/vector_util.hpp>
#include <sfg/math/math_common.hpp>

#include <cstddef>
#include <type_traits>

namespace sfg
{
	class chunk_allocator_t
	{
	public:
		~chunk_allocator_t();

		void			 init(size_t total_size);
		void			 uninit();
		void			 reset();
		chunk_handle32_t allocate_bytes(size_t size, size_t alignment);
		chunk_handle32_t allocate_text(const char* src);
		const char*		 get_text(chunk_handle32_t handle);

		template <typename T> inline chunk_handle32_t allocate(size_t count)
		{
			static_assert(std::is_trivially_copyable_v<T>, "chunk_allocator_t typed allocation only supports trivially copyable types");
			static_assert(std::is_trivially_destructible_v<T>, "chunk_allocator_t typed allocation only supports trivially destructible types");

			SFG_ASSERT(count != 0);

			const size_t item_alignment	  = alignof(T);
			const size_t padded_item_size = ALIGN_UP(sizeof(T), item_alignment);
			SFG_ASSERT(padded_item_size <= UINT32_MAX);
			SFG_ASSERT(count <= UINT32_MAX / padded_item_size);

			const chunk_handle32_t ret = allocate_bytes(padded_item_size * count, item_alignment);
			T*					   ptr = reinterpret_cast<T*>(_raw + ret.head);
			for (size_t i = 0; i < count; ++i)
				std::construct_at(&ptr[i]);

			return ret;
		}

		template <typename T> inline chunk_handle32_t allocate(size_t count, T*& out)
		{
			const chunk_handle32_t ret = allocate<T>(count);
			out						   = get<T>(ret);
			return ret;
		}

		inline void free(chunk_handle32_t handle)
		{
			SFG_ASSERT(handle.size != 0);
			SFG_MEMSET(_raw + handle.head, 0, handle.size); // optional
			insert_free_chunk_sorted(handle);
		}

		template <typename T> T* get(chunk_handle32_t handle)
		{
			SFG_ASSERT(handle.size != 0);
			return reinterpret_cast<T*>(_raw + handle.head);
		}

		template <typename T> T* get(chunk_handle32_t handle) const
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

		inline u32 get_capacity() const
		{
			return _total_size;
		}

	private:
		// Insert while keeping order and coalescing neighbors.
		inline void insert_free_chunk_sorted(chunk_handle32_t c)
		{
			auto it = std::lower_bound(_free_chunks.begin(), _free_chunks.end(), c, [](const chunk_handle32_t& a, const chunk_handle32_t& b) { return a.head < b.head; });

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
		u8*						   _raw = nullptr;
		vector_t<chunk_handle32_t> _free_chunks; // ALWAYS kept sorted by head
		u32						   _head	   = 0;
		u32						   _total_size = 0;
	};

	using chunk_allocator32_t = chunk_allocator_t;
}
