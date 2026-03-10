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

#include "char_util.hpp"
#include "memory/memory.hpp"

#include <cstring>
#include <cstdio>
#include <charconv>

namespace SFG
{
	static inline size_t remaining_bytes(char* cur, char* end)
	{
		return (end > cur) ? static_cast<size_t>(end - cur) : 0u;
	}

	static inline void null_terminate_in_place(char* cur, char* end)
	{
		if (cur < end)
			*cur = '\0';
		else if (end)
			*(end - 1) = '\0';
	}

	namespace char_util
	{
		bool append(char*& cur, char* end, const char* data, size_t len)
		{
			if (len == 0)
				return true;

			size_t avail = remaining_bytes(cur, end);
			if (avail < (len + 1))
				return false;

			SFG_MEMCPY(cur, data, len);
			cur += len;
			null_terminate_in_place(cur, end);
			return true;
		}

		bool append(char*& cur, char* end, const char* cstr)
		{
			if (!cstr)
				return false;
			return append(cur, end, cstr, std::strlen(cstr));
		}

		bool append_char(char*& cur, char* end, char c)
		{
			size_t avail = remaining_bytes(cur, end);
			if (avail < 2)
				return false;

			*cur++ = c;
			null_terminate_in_place(cur, end);
			return true;
		}

		bool append_i32(char*& cur, char* end, i32 v)
		{
			return append_i64(cur, end, static_cast<i64>(v));
		}

		bool append_u32(char*& cur, char* end, u32 v)
		{
			return append_u64(cur, end, static_cast<u64>(v));
		}

		bool append_i64(char*& cur, char* end, i64 v)
		{
			if (remaining_bytes(cur, end) < 2)
				return false;

			char* out_begin = cur;
			char* out_end	= end ? (end - 1) : nullptr;

			auto r = std::to_chars(out_begin, out_end, v);
			if (r.ec != std::errc{})
				return false;

			cur = r.ptr;
			null_terminate_in_place(cur, end);
			return true;
		}

		bool append_u64(char*& cur, char* end, u64 v)
		{
			if (remaining_bytes(cur, end) < 2)
				return false;

			char* out_begin = cur;
			char* out_end	= end ? (end - 1) : nullptr;

			auto r = std::to_chars(out_begin, out_end, v);
			if (r.ec != std::errc{})
				return false;

			cur = r.ptr;
			null_terminate_in_place(cur, end);
			return true;
		}

		bool append_double(char*& cur, char* end, double v, int precision)
		{
			char	  tmp[128];
			const int n = std::snprintf(tmp, sizeof(tmp), "%.*f", precision, v);
			if (n <= 0)
				return false;

			return append(cur, end, tmp, static_cast<size_t>(n));
		}

		bool appendf_va(char*& cur, char* end, const char* fmt, va_list args)
		{
			if (!fmt)
				return false;

			size_t avail = remaining_bytes(cur, end);
			if (avail < 2)
				return false;

			va_list args_copy;
#if defined(_MSC_VER)
			args_copy = args;
#else
			va_copy(args_copy, args);
#endif

			const int wrote = std::vsnprintf(cur, avail, fmt, args_copy);

#if !defined(_MSC_VER)
			va_end(args_copy);
#endif

			if (wrote < 0)
				return false;
			if (static_cast<size_t>(wrote) >= avail)
				return false;

			cur += static_cast<size_t>(wrote);
			null_terminate_in_place(cur, end);
			return true;
		}

		void replace_all(char* c, char to_replace, char replacement)
		{
			if (!c)
				return;

			while (*c)
			{
				if (*c == to_replace)
					*c = replacement;
				++c;
			}
		}
	}
}
