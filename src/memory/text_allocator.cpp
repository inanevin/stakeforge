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
#include "io/assert.hpp"
#include "data/vector_util.hpp"
#include <cstring>

namespace SFG
{
	void text_allocator::init(u32 capacity)
	{
		_raw	  = new char[capacity];
		_capacity = capacity;
		_free_list.reserve(_capacity / 32);

		if (_raw)
			SFG_MEMSET(_raw, 0, capacity);
	}

	void text_allocator::uninit()
	{
		delete[] _raw;
		_raw	  = nullptr;
		_capacity = 0;
		_free_list.clear();
	}

	const char* text_allocator::allocate(size_t len)
	{
		const size_t need = len + 1;

		auto it = vector_util::find_if(_free_list, [need](const allocation& alloc) { return alloc.size >= need; });

		if (it != _free_list.end())
		{
			allocation& free = *it;

			char* result = free.ptr;
			if (free.size == need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += need;
				free.size -= need;
			}

			result[need - 1] = '\0';
			return result;
		}

		// fallback to bump allocation
		SFG_ASSERT(_head + need <= _capacity);
		if (_head + need > _capacity)
			return nullptr;

		char* allocated = &_raw[_head];
		_head += need;
		allocated[need - 1] = '\0';
		return allocated;
	}

	const char* text_allocator::allocate(const char* text, size_t len)
	{
		if (!text)
			return nullptr;

		const size_t txt_sz = std::strlen(text) + 1;
		const size_t need	= txt_sz > len ? txt_sz : len;

		auto it = vector_util::find_if(_free_list, [need](const allocation& alloc) { return alloc.size >= need; });

		if (it != _free_list.end())
		{
			allocation& free = *it;

			char* result = free.ptr;
			if (free.size == need)
			{
				_free_list.erase(it);
			}
			else
			{
				free.ptr += need;
				free.size -= need;
			}

			std::memcpy(result, text, need);
			return result;
		}

		if (_head + need > _capacity)
			return nullptr;

		char* allocated = &_raw[_head];
		std::memcpy(allocated, text, need);
		_head += need;
		return allocated;
	}

	void text_allocator::deallocate(char* ptr)
	{
		if (!ptr)
			return;
		_free_list.push_back({
			.ptr  = ptr,
			.size = std::strlen(ptr) + 1,
		});
	}

	void text_allocator::deallocate(const char* ptr)
	{
		if (!ptr)
			return;
		_free_list.push_back({
			.ptr  = const_cast<char*>(ptr),
			.size = std::strlen(ptr) + 1,
		});
	}

}
