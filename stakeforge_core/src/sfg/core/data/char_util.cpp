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

#include "core/data/char_util.hpp"
#include "core/memory/memory.hpp"

namespace sfg
{
	namespace char_util
	{
		bool append(char*& cur, char* end, const char* data, size_t len)
		{
			if (len == 0)
				return true;

			const size_t avail = static_cast<size_t>(end - cur);

			if (len >= avail)
				return false;

			SFG_MEMCPY(cur, data, len);
			cur += len;
			*cur = '\0';

			return true;
		}

		bool append(char*& cur, char* end, const char* cstr)
		{
			return append(cur, end, cstr, std::strlen(cstr));
		}

		bool append_char(char*& cur, char* end, char c)
		{
			const size_t avail = static_cast<size_t>(end - cur);

			if (avail < 2)
				return false;

			*cur++ = c;
			*cur   = '\0';

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
			if (end - cur < 2)
				return false;

			const auto r = std::to_chars(cur, end - 1, v);

			if (r.ec != std::errc{})
				return false;

			cur	 = r.ptr;
			*cur = '\0';

			return true;
		}

		bool append_u64(char*& cur, char* end, u64 v)
		{
			if (end - cur < 2)
				return false;

			const auto r = std::to_chars(cur, end - 1, v);

			if (r.ec != std::errc{})
				return false;

			cur	 = r.ptr;
			*cur = '\0';

			return true;
		}

		bool append_double(char*& cur, char* end, double v, int precision)
		{
			char	  tmp[128] = {};
			const int n		   = std::snprintf(tmp, sizeof(tmp), "%.*f", precision, v);

			if (n < 0 || static_cast<size_t>(n) >= sizeof(tmp))
				return false;

			return append(cur, end, tmp, static_cast<size_t>(n));
		}

		bool appendf_va(char*& cur, char* end, const char* fmt, va_list args)
		{
			const size_t avail = static_cast<size_t>(end - cur);

			if (avail < 2)
				return false;

			va_list args_copy = {};
#if defined(_MSC_VER)
			args_copy = args;
#else
			va_copy(args_copy, args);
#endif

			const int wrote = std::vsnprintf(cur, avail, fmt, args_copy);

#if !defined(_MSC_VER)
			va_end(args_copy);
#endif

			if (wrote < 0 || static_cast<size_t>(wrote) >= avail)
				return false;

			cur += static_cast<size_t>(wrote);

			return true;
		}

		void replace_all(char* c, char to_replace, char replacement)
		{
			while (*c)
			{
				if (*c == to_replace)
					*c = replacement;

				++c;
			}
		}
	}
}
