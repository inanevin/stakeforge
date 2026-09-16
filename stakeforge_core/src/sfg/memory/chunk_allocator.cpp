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
		SFG_ASSERT(_page_size == 0);
	}

	void chunk_allocator_t::init(size_t page_size, chunk_page_policy_e policy)
	{
		SFG_ASSERT(_page_size == 0);
		SFG_ASSERT(page_size != 0);
		SFG_ASSERT(page_size <= UINT32_MAX - (alignof(std::max_align_t) - 1));

		_page_size = static_cast<u32>(ALIGN_UP(page_size, alignof(std::max_align_t)));
		_policy	   = policy;

		allocate_page(_page_size);
	}

	void chunk_allocator_t::uninit()
	{
		SFG_ASSERT(_page_size != 0);

		for (u32 i = 0; i < _pages.size(); ++i)
		{
			if (_pages[i].raw != nullptr)
				release_page(i);
		}

		vector_t<page_t>{}.swap(_pages);
		_page_size = 0;
		_free_page = UINT32_MAX;
	}

	void chunk_allocator_t::reset()
	{
		SFG_ASSERT(_page_size != 0);

		for (u32 i = 0; i < _pages.size(); ++i)
		{
			page_t& page = _pages[i];

			if (page.raw == nullptr)
				continue;

			if (_policy == chunk_page_policy_e::release)
			{
				release_page(i);
				continue;
			}

			page.free_chunks.resize(0);
			page.head			  = 0;
			page.live_allocations = 0;
		}
	}

	u32 chunk_allocator_t::allocate_page(u32 size)
	{
		const size_t capacity = ALIGN_UP(static_cast<size_t>(size), alignof(std::max_align_t));
		u8*			 raw	  = reinterpret_cast<u8*>(SFG_ALIGNED_MALLOC(alignof(std::max_align_t), capacity));

		SFG_FAIL(raw != nullptr, "failed to allocate chunk allocator page");

		u32 index = _free_page;

		if (index == UINT32_MAX)
		{
			SFG_FAIL(_pages.size() < UINT32_MAX, "chunk allocator page indices exhausted");

			index = static_cast<u32>(_pages.size());
			_pages.emplace_back();
		}
		else
		{
			_free_page = _pages[index].next_free_page;
		}

		page_t& page = _pages[index];

		page.raw			= raw;
		page.capacity		= static_cast<u32>(capacity);
		page.next_free_page = UINT32_MAX;
		_capacity += capacity;

		return index;
	}

	void chunk_allocator_t::release_page(u32 index)
	{
		page_t& page = _pages[index];

		SFG_ALIGNED_FREE(page.raw);
		_capacity -= page.capacity;

		vector_t<free_chunk_t>{}.swap(page.free_chunks);
		page.raw			  = nullptr;
		page.head			  = 0;
		page.capacity		  = 0;
		page.live_allocations = 0;
		page.next_free_page	  = _free_page;
		_free_page			  = index;
	}

	chunk_handle32_t chunk_allocator_t::allocate_bytes(size_t size, size_t alignment)
	{
		SFG_ASSERT(_page_size != 0);
		SFG_ASSERT(size != 0);
		SFG_ASSERT(alignment != 0);
		SFG_ASSERT((alignment & (alignment - 1)) == 0);
		SFG_ASSERT(alignment <= alignof(std::max_align_t));
		SFG_ASSERT(size <= UINT32_MAX - (alignof(std::max_align_t) - 1));

		const u32 requested_size = static_cast<u32>(size);

		for (u32 index = 0; index < _pages.size(); ++index)
		{
			page_t& page = _pages[index];

			if (page.raw == nullptr || page.capacity < requested_size)
				continue;

			for (auto it = page.free_chunks.begin(); it != page.free_chunks.end(); ++it)
			{
				const free_chunk_t chunk		= *it;
				const u32		   aligned_head = ALIGN_UP(chunk.head, static_cast<u32>(alignment));
				const u32		   padding		= aligned_head - chunk.head;

				if (padding > chunk.size || requested_size > chunk.size - padding)
					continue;

				page.free_chunks.erase(it);

				if (padding != 0)
					insert_free_chunk_sorted(page, {.head = chunk.head, .size = padding});

				const u32 remaining_size = chunk.size - padding - requested_size;

				if (remaining_size != 0)
					insert_free_chunk_sorted(page, {.head = aligned_head + requested_size, .size = remaining_size});

				++page.live_allocations;

				return {.page = index, .head = aligned_head, .size = requested_size};
			}

			const u32 aligned_head = ALIGN_UP(page.head, static_cast<u32>(alignment));

			if (requested_size > page.capacity - aligned_head)
				continue;

			if (aligned_head != page.head)
				insert_free_chunk_sorted(page, {.head = page.head, .size = aligned_head - page.head});

			page.head = aligned_head + requested_size;
			++page.live_allocations;

			return {.page = index, .head = aligned_head, .size = requested_size};
		}

		const u32 index = allocate_page(std::max(_page_size, requested_size));
		page_t&	  page	= _pages[index];

		page.head			  = requested_size;
		page.live_allocations = 1;

		return {.page = index, .head = 0, .size = requested_size};
	}

	void chunk_allocator_t::free(chunk_handle32_t handle)
	{
		SFG_ASSERT(handle.size != 0);

		page_t& page = _pages[handle.page];

		SFG_MEMSET(page.raw + handle.head, 0, handle.size); // optional
		--page.live_allocations;

		if (page.live_allocations == 0)
		{
			if (_policy == chunk_page_policy_e::release)
				release_page(handle.page);
			else
			{
				page.free_chunks.resize(0);
				page.head = 0;
			}

			return;
		}

		insert_free_chunk_sorted(page, {.head = handle.head, .size = handle.size});

		const free_chunk_t tail = page.free_chunks.back();

		if (tail.head + tail.size == page.head)
		{
			page.head = tail.head;
			page.free_chunks.pop_back();
		}
	}

	void chunk_allocator_t::insert_free_chunk_sorted(page_t& page, free_chunk_t chunk)
	{
		auto& chunks = page.free_chunks;
		auto  it	 = std::lower_bound(chunks.begin(), chunks.end(), chunk, [](const free_chunk_t& a, const free_chunk_t& b) { return a.head < b.head; });

		it = chunks.insert(it, chunk); // insert c at sorted position

		// Merge with previous if adjacent
		if (it != chunks.begin())
		{
			auto prev = it - 1;

			if (prev->head + prev->size == it->head)
			{
				prev->size += it->size;
				it = chunks.erase(it); // drop current, keep prev
				it = prev;			   // iterator now at merged block
			}
		}

		// Merge with next if adjacent
		if (it + 1 != chunks.end())
		{
			auto next = it + 1;

			if (it->head + it->size == next->head)
			{
				it->size += next->size;
				chunks.erase(next);
			}
		}
	}

	chunk_handle32_t chunk_allocator_t::allocate_text(const char* src)
	{
		const size_t		   len	  = strlen(src);
		const chunk_handle32_t handle = allocate<u8>(len + 1);
		char*				   dst	  = get<char>(handle);

		SFG_MEMCPY(dst, src, len);
		dst[len] = '\0';

		return handle;
	}

	const char* chunk_allocator_t::get_text(chunk_handle32_t handle)
	{
		if (handle.size == 0)
			return nullptr;

		return get<char>(handle);
	}
}
