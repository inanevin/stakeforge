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

#include "core/common/sfg_exports.hpp"

#include "core/data/vector.hpp"
#include "core/data/string.hpp"
#include "core/common/size_definitions.hpp"
#include <cctype>

namespace sfg
{
	namespace string_util
	{
		SFG_API string_t			 remove_all_except_first(const string_t& str, const string_t& delimiter);
		SFG_API int					 append_float(f32 value, char* target_bufffer, u32 max_chars, u32 decimals, bool null_term);
		SFG_API void				 replace_all(string_t& str, const string_t& to_replace, const string_t& replacement);
		SFG_API void				 to_upper(string_t& str);
		SFG_API void				 to_lower(string_t& str);
		SFG_API bool				 contains_case_insensitive(const char* text, size_t text_size, const char* search, size_t search_size);
		SFG_API string_t			 to_pascal_case(const char* str);
		template <class STRING> void to_lower(STRING& input)
		{
			for (char& c : input)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		SFG_API void		   remove_whitespace(string_t& str);
		SFG_API wstring_t	   to_wstr(const string_t& string_t);
		SFG_API void		   split(vector_t<string_t>& out, const string_t& str, const string_t& split);
		SFG_API char*		   wchar_to_char(const wchar_t* wch);
		SFG_API const wchar_t* char_to_wchar(const char* ch);
		SFG_API bool		   to_float(const string_t& str, f32& out_f, u32& out_decimals, char seperator = '.');
		SFG_API bool		   to_int(const string_t& str, int& out_i);
		SFG_API bool		   to_big_uint(const string_t& str, u64& out_i);
		SFG_API bool		   to_bool(const string_t& str, bool& out_b);
	}

}
