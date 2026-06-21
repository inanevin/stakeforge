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

#include "text_allocator.hpp"
#include "memory.hpp"
#include <sfg/data/vector_util.hpp>
#include <sfg/io/assert.hpp>
#include <algorithm>
#include <cstring>
#include <limits>

namespace sfg
{
	namespace
	{
		constexpr size_t ALLOCATION_HEADER_SIZE = sizeof(u32);

		void write_block_size(char* ptr, u32 size)
		{
			SFG_MEMCPY(ptr, &size, sizeof(size));
		}

		u32 read_block_size(const char* ptr)
		{
			u32 size = 0;
			SFG_MEMCPY(&size, ptr, sizeof(size));
			return size;
		}

		char* payload_from_block(char* block)
		{
			return block + ALLOCATION_HEADER_SIZE;
		}

		const char* header_from_payload(const char* payload)
		{
			return payload - ALLOCATION_HEADER_SIZE;
		}
	}

	void text_allocator_t::init(u32 capacity)
	{
		_raw	  = new char[capacity];
		_capacity = capacity;
		_free_list.reserve(_capacity / 32);

		if (_raw)
			SFG_MEMSET(_raw, 0, capacity);
	}

	void text_allocator_t::uninit()
	{
		delete[] _raw;
		_raw	  = nullptr;
		_capacity = 0;
		_free_list.clear();
	}

	const char* text_allocator_t::allocate(size_t len)
	{
		const size_t payload_need = len + 1;
		const size_t block_need	  = ALLOCATION_HEADER_SIZE + payload_need;
		SFG_ASSERT(block_need <= std::numeric_limits<u32>::max());

		auto it = vector_util::find_if(_free_list, [block_need](const allocation_t& alloc) { return alloc.size >= block_need; });

		if (it != _free_list.end())
		{
			allocation_t& free = *it;

			char* block = free.ptr;
			if (free.size == block_need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += block_need;
				free.size -= block_need;
			}

			write_block_size(block, static_cast<u32>(block_need));
			char* result			 = payload_from_block(block);
			result[payload_need - 1] = '\0';
			return result;
		}

		// fallback to bump allocation
		SFG_ASSERT(_head + block_need <= _capacity);
		if (_head + block_need > _capacity)
			return nullptr;

		char* block = &_raw[_head];
		_head += static_cast<u32>(block_need);
		write_block_size(block, static_cast<u32>(block_need));

		char* allocated				= payload_from_block(block);
		allocated[payload_need - 1] = '\0';
		return allocated;
	}

	const char* text_allocator_t::allocate(const char* text, size_t len)
	{
		if (!text)
			return nullptr;

		const size_t txt_sz		  = std::strlen(text) + 1;
		const size_t payload_need = txt_sz > len ? txt_sz : len;
		const size_t block_need	  = ALLOCATION_HEADER_SIZE + payload_need;
		SFG_ASSERT(block_need <= std::numeric_limits<u32>::max());

		auto it = vector_util::find_if(_free_list, [block_need](const allocation_t& alloc) { return alloc.size >= block_need; });

		if (it != _free_list.end())
		{
			allocation_t& free = *it;

			char* block = free.ptr;
			if (free.size == block_need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += block_need;
				free.size -= block_need;
			}

			write_block_size(block, static_cast<u32>(block_need));
			char* result = payload_from_block(block);
			std::memcpy(result, text, txt_sz);
			if (payload_need > txt_sz)
				SFG_MEMSET(result + txt_sz, 0, payload_need - txt_sz);
			return result;
		}

		if (_head + block_need > _capacity)
			return nullptr;

		char* block = &_raw[_head];
		write_block_size(block, static_cast<u32>(block_need));
		char* allocated = payload_from_block(block);
		std::memcpy(allocated, text, txt_sz);
		if (payload_need > txt_sz)
			SFG_MEMSET(allocated + txt_sz, 0, payload_need - txt_sz);

		_head += static_cast<u32>(block_need);
		return allocated;
	}

	void text_allocator_t::deallocate(char* ptr)
	{
		if (!ptr)
			return;

		char* header = const_cast<char*>(header_from_payload(ptr));
		SFG_ASSERT(header >= _raw && header < _raw + _capacity);
		insert_free_allocation_sorted({
			.ptr  = header,
			.size = read_block_size(header),
		});
	}

	void text_allocator_t::deallocate(const char* ptr)
	{
		if (!ptr)
			return;

		const char* header = header_from_payload(ptr);
		SFG_ASSERT(header >= _raw && header < _raw + _capacity);
		insert_free_allocation_sorted({
			.ptr  = const_cast<char*>(header),
			.size = read_block_size(header),
		});
	}

	void text_allocator_t::insert_free_allocation_sorted(allocation_t allocation)
	{
		auto it = std::lower_bound(_free_list.begin(), _free_list.end(), allocation, [](const allocation_t& a, const allocation_t& b) { return a.ptr < b.ptr; });

		it = _free_list.insert(it, allocation);

		if (it != _free_list.begin())
		{
			auto prev = it - 1;
			if (prev->ptr + prev->size == it->ptr)
			{
				prev->size += it->size;
				it = _free_list.erase(it);
				it = prev;
			}
		}

		if (it + 1 != _free_list.end())
		{
			auto next = it + 1;
			if (it->ptr + it->size == next->ptr)
			{
				it->size += next->size;
				_free_list.erase(next);
			}
		}
	}

}
