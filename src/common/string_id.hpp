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
	using string_id = u64;

	constexpr string_id hash_bytes(const char* str, size_t len) noexcept
	{
		string_id h = 1469598103934665603ull;
		for (size_t i = 0; i < len; ++i)
			h = (h ^ static_cast<unsigned char>(str[i])) * 1099511628211ull;
		return h;
	}

	// compile-time for string literals (covers #T)
	template <size_t N> consteval string_id to_sid(const char (&lit)[N]) noexcept
	{
		static_assert(N > 0);
		// N includes '\0'
		return hash_bytes(lit, N - 1);
	}

	// runtime for C-strings
	constexpr string_id to_sid(const char* s) noexcept
	{
		// no pointer casts => friendlier to constexpr rules too
		string_id h = 1469598103934665603ull;
		for (size_t i = 0; s[i] != '\0'; ++i)
			h = (h ^ static_cast<unsigned char>(s[i])) * 1099511628211ull;
		return h;
	}

	// string-like: has data() and size()
	template <class S> constexpr auto to_sid(const S& s) noexcept -> decltype(s.data(), s.size(), string_id{})
	{
		return hash_bytes(s.data(), static_cast<size_t>(s.size()));
	}

	constexpr string_id operator"" _hs(const char* str, size_t len) noexcept
	{
		return hash_bytes(str, len);
	}

#define TO_SID(X)  ::SFG::to_sid((X))
#define TO_SIDC(X) (X##_hs)
}
