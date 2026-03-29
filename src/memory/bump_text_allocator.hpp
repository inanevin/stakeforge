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

namespace SFG
{
	class bump_text_allocator
	{
	public:
		struct string_view_t
		{
			const char* ptr;
			size_t		sz;

			inline bool empty()
			{
				return sz == 0;
			}

			inline const char* data()
			{
				return ptr;
			}

			inline size_t size()
			{
				return sz;
			}
		};

		bump_text_allocator() = default;
		~bump_text_allocator();

		bump_text_allocator(const bump_text_allocator&)			   = delete;
		bump_text_allocator& operator=(const bump_text_allocator&) = delete;

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(size_t capacity_bytes);
		void uninit();
		void reset();

		// -----------------------------------------------------------------------------
		// string building
		// -----------------------------------------------------------------------------

		const char* allocate_reserve(size_t reserve_bytes_including_null);
		const char* allocate(const char* initial_text, size_t reserve_extra = 0);
		const char* terminate();
		const char* current_c_str() const;
		size_t		remaining() const;

		// -----------------------------------------------------------------------------
		// append utilities
		// -----------------------------------------------------------------------------

		bool append(string_view_t s);
		bool append(const char* s);
		bool append(char c);
		bool append(i32 v);
		bool append(u32 v);
		bool append(i64 v);
		bool append(u64 v);
		bool append(double v, int precision = 3);
		bool appendf(const char* fmt, ...);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		size_t capacity() const
		{
			return _cap;
		}
		size_t head() const
		{
			return _head;
		}
		const char* raw() const
		{
			return _raw;
		}

	private:
		bool append_i64(i64 v);
		bool append_u64(u64 v);
		bool ensure_space(size_t bytes_needed_including_null) const;
		void null_terminate_in_place();

	private:
		char*  _raw	 = nullptr;
		size_t _cap	 = 0;
		size_t _head = 0;

		// Active string state
		char* _cur_start = nullptr;
		char* _cur		 = nullptr;
		char* _cur_end	 = nullptr; // one-past last byte owned by active reservation
	};
}
