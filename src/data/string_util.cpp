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

#include "string_util.hpp"
#include "memory/memory.hpp"
#include "io/assert.hpp"
#include <charconv>
#include <codecvt>
#include <locale>
#include <iostream>
#include <cwchar>
#include <cstring>
#include <algorithm>

#ifdef SFG_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4333)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

namespace SFG
{

	wstring string_util::to_wstr(const string& string)
	{
		std::string												  str = string.c_str();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		return converter.from_bytes(str);
	}

	char* string_util::wchar_to_char(const wchar_t* wch)
	{
		// Count required buffer size (plus one for null-terminator).
		size_t size	  = (wcslen(wch) + 1) * sizeof(wchar_t);
		char*  buffer = new char[size];

#ifdef __STDC_LIB_EXT1__
		// wcstombs_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined
		size_t convertedSize;
		std::wcstombs_s(&convertedSize, buffer, size, input, size);
#else
#pragma warning(disable : 4996)
		std::wcstombs(buffer, wch, size);
#endif
		return buffer;
	}

	const wchar_t* string_util::char_to_wchar(const char* ch)
	{
#ifdef SFG_PLATFORM_WINDOWS
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring									 wide_str = converter.from_bytes(ch);

		wchar_t* wide_copy = new wchar_t[wide_str.size() + 1];
		wcscpy_s(wide_copy, wide_str.size() + 1, wide_str.c_str());

		return wide_copy;
#endif

#ifdef SFG_PLATFORM_OSX
		// Convert the input char string to a wchar_t string on Apple platform
		size_t	 length	   = strlen(ch);
		wchar_t* wide_copy = new wchar_t[length + 1];

		mbstowcs(wide_copy, ch, length);
		wide_copy[length] = L'\0'; // Null-terminate the wide string

		return wide_copy;
#endif
	}

	void string_util::replace_all(string& str, const string& to_replace, const string& replacement)
	{
		if (to_replace.empty())
			return;

		std::string result;
		result.reserve(str.size());

		size_t pos = 0;
		while (pos < str.size())
		{
			size_t found = str.find(to_replace, pos);
			if (found == std::string::npos)
			{
				result.append(str, pos, str.size() - pos);
				break;
			}
			result.append(str, pos, found - pos);
			result.append(replacement);
			pos = found + to_replace.size();
		}
		str = result;
	}

	bool string_util::to_float(const string& str, f32& out_f, u32& outDecimals, char seperator)
	{
		try
		{
			std::size_t pos = str.find(seperator);
			if (pos != std::string::npos)
				outDecimals = static_cast<u32>(str.length() - pos - 1);

			out_f = std::stof(str);
			return true;
		}
		catch (const std::exception& e)
		{
			return false;
		}
	}

	bool string_util::to_int(const string& str, int& out_i)
	{
		try
		{
			out_i = std::stoi(str);
			return true;
		}
		catch (const std::exception& e)
		{
			return false;
		}
	}

	bool string_util::to_big_uint(const string& str, u64& out_i)
	{
		try
		{
			out_i = static_cast<u64>(std::stoull(str));
			return true;
		}
		catch (const std::exception& e)
		{
			return false;
		}
	}

	string string_util::remove_all_except_first(const string& str, const string& delimiter)
	{
		string		result = str;
		std::size_t pos	   = result.find(delimiter); // find the first dot

		// if there is a dot in the string
		if (pos != std::string::npos)
		{
			// remove all subsequent dots
			pos++; // start from the next character
			std::size_t next;
			while ((next = result.find('.', pos)) != std::string::npos)
			{
				result.erase(next, 1); // erase the dot
			}
		}

		return result;
	}

	int string_util::append_float(f32 value, char* target_buffer, u32 max_chars, u32 decimals, bool null_term)
	{
		SFG_ASSERT(decimals < max_chars);
		SFG_ASSERT(max_chars <= 16);
		int	 written = 0;
		char float_buf[16];

		for (int precision = decimals; precision >= 0; --precision)
		{
			written = snprintf(float_buf, sizeof(float_buf), "%.*f", precision, value);
			if (written <= static_cast<int>(max_chars))
				break;
		}

		SFG_MEMCPY(target_buffer, float_buf, written);

		if (null_term)
			target_buffer[written] = '\0';
		return written;
	}

	void string_util::split(vector<string>& out, const string& str, const string& split)
	{
		// Split the path into directories
		size_t start = 0, end = str.find(split.c_str());
		while (end != string::npos)
		{
			const auto aq = str.substr(start, end - start);
			out.push_back(aq);
			start = end + split.size();
			end	  = str.find(split.c_str(), start);
		}
		out.push_back(str.substr(start));
	}

	void string_util::to_lower(string& input)
	{
		for (char& c : input)
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	void string_util::to_upper(string& input)
	{
		for (char& c : input)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}

	void string_util::remove_whitespace(string& str)
	{
		size_t write = 0;
		for (size_t read = 0; read < str.size(); ++read)
		{
			if (!std::isspace(static_cast<unsigned char>(str[read])))
			{
				str[write++] = str[read];
			}
		}
		str.resize(write);
	}

}

#ifdef SFG_COMPILER_MSVC
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
