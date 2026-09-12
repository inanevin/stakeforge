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

#include <sfg/common/size_definitions.hpp>
#include <cstdarg>

namespace sfg
{
	namespace char_util
	{
		bool append(char*& cur, char* end, const char* data, size_t len);
		bool append(char*& cur, char* end, const char* cstr);
		bool append_char(char*& cur, char* end, char c);
		bool append_i32(char*& cur, char* end, i32 v);
		bool append_u32(char*& cur, char* end, u32 v);
		bool append_i64(char*& cur, char* end, i64 v);
		bool append_u64(char*& cur, char* end, u64 v);
		bool append_double(char*& cur, char* end, double v, int precision);
		bool appendf_va(char*& cur, char* end, const char* fmt, va_list args);
		void replace_all(char* c, char to_replace, char replacement);
	}
}
