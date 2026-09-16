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

#include <cstddef>
#include <type_traits>

namespace sfg
{
	enum class chunk_page_policy_e : u8
	{
		keep,
		release,
	};

	class chunk_allocator_t final
	{
	public:
		chunk_allocator_t() = default;
		~chunk_allocator_t();
		chunk_allocator_t(const chunk_allocator_t& other)			 = delete;
		chunk_allocator_t& operator=(const chunk_allocator_t& other) = delete;

		void			 init(size_t page_size, chunk_page_policy_e policy = chunk_page_policy_e::keep);
		void			 uninit();
		void			 reset();
		chunk_handle32_t allocate_bytes(size_t size, size_t alignment);
		chunk_handle32_t allocate_text(const char* src);
		const char*		 get_text(chunk_handle32_t handle);
		void			 free(chunk_handle32_t handle);

		template <typename T> inline chunk_handle32_t allocate(size_t count)
		{
			static_assert(std::is_trivially_copyable_v<T>, "chunk_allocator_t typed allocation only supports trivially copyable types");
			static_assert(std::is_trivially_destructible_v<T>, "chunk_allocator_t typed allocation only supports trivially destructible types");

			SFG_ASSERT(count != 0);
			SFG_ASSERT(sizeof(T) <= UINT32_MAX);
			SFG_ASSERT(count <= UINT32_MAX / sizeof(T));

			const chunk_handle32_t ret = allocate_bytes(sizeof(T) * count, alignof(T));
			T*					   ptr = get<T>(ret);

			for (size_t i = 0; i < count; ++i)
				std::construct_at(&ptr[i]);

			return ret;
		}

		template <typename T> inline chunk_handle32_t allocate(size_t count, T*& out)
		{
			const chunk_handle32_t ret = allocate<T>(count);

			out = get<T>(ret);

			return ret;
		}

		template <typename T> T* get(chunk_handle32_t handle)
		{
			SFG_ASSERT(handle.size != 0);

			return reinterpret_cast<T*>(_pages[handle.page].raw + handle.head);
		}

		template <typename T> T* get(chunk_handle32_t handle) const
		{
			SFG_ASSERT(handle.size != 0);

			return reinterpret_cast<T*>(_pages[handle.page].raw + handle.head);
		}

		inline size_t get_capacity() const
		{
			return _capacity;
		}

	private:
		struct free_chunk_t
		{
			u32 head = 0;
			u32 size = 0;
		};

		struct page_t
		{
			vector_t<free_chunk_t> free_chunks		= {}; // ALWAYS kept sorted by head
			u8*					   raw				= nullptr;
			u32					   head				= 0;
			u32					   capacity			= 0;
			u32					   live_allocations = 0;
			u32					   next_free_page	= UINT32_MAX;
		};

		u32	 allocate_page(u32 size);
		void release_page(u32 index);
		// Insert while keeping order and coalescing neighbors.
		void insert_free_chunk_sorted(page_t& page, free_chunk_t chunk);

		vector_t<page_t>	_pages	   = {};
		size_t				_capacity  = 0;
		u32					_page_size = 0;
		u32					_free_page = UINT32_MAX;
		chunk_page_policy_e _policy	   = chunk_page_policy_e::keep;
	};

	using chunk_allocator32_t = chunk_allocator_t;
}
