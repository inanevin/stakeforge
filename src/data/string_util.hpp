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
#include "data/string.hpp"

namespace SFG
{
	namespace string_util
	{
		string		   remove_all_except_first(const string& str, const string& delimiter);
		int			   append_float(f32 value, char* target_bufffer, u32 max_chars, u32 decimals, bool null_term);
		void		   replace_all(string& str, const string& to_replace, const string& replacement);
		void		   to_upper(string& str);
		void		   to_lower(string& str);
		void		   remove_whitespace(string& str);
		wstring		   to_wstr(const string& string);
		void		   split(vector<string>& out, const string& str, const string& split);
		char*		   wchar_to_char(const wchar_t* wch);
		const wchar_t* char_to_wchar(const char* ch);
		bool		   to_float(const string& str, f32& out_f, u32& out_decimals, char seperator = '.');
		bool		   to_int(const string& str, int& out_i);
		bool		   to_big_uint(const string& str, u64& out_i);
	}

}
