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
#include "data/vector.hpp"

namespace SFG
{
	class text_allocator_t
	{

	private:
		struct allocation_t
		{
			char*  ptr	= nullptr;
			size_t size = 0;
		};

	public:
		text_allocator_t() : _head(0){};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(u32 capacity);
		void uninit();

		// -----------------------------------------------------------------------------
		// memory api
		// -----------------------------------------------------------------------------

		const char* allocate(size_t len);
		const char* allocate(const char* text, size_t len = 0);
		void		deallocate(char* ptr);
		void		deallocate(const char* ptr);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline constexpr size_t get_capacity() const
		{
			return _capacity;
		}

		inline constexpr size_t get_head() const
		{
			return _head;
		}

		inline char* get_raw() const
		{
			return _raw;
		}

		inline void reset()
		{
			_free_list.resize(0);
			_head = 0;
		}

	private:
		vector_t<allocation_t> _free_list;
		char*				   _raw		 = nullptr;
		u32					   _head	 = 0;
		u32					   _capacity = 0;
	};

}
