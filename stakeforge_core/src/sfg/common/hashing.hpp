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
#include "size_definitions.hpp"

namespace sfg
{
	class hashing_t
	{
	public:
		static constexpr sid_t hash_bytes(const char* str, size_t len) noexcept
		{
			sid_t h = 1469598103934665603ull;
			for (size_t i = 0; i < len; ++i)
				h = (h ^ static_cast<unsigned char>(str[i])) * 1099511628211ull;
			return h;
		}

		template <size_t N> static consteval sid_t to_sid(const char (&lit)[N]) noexcept
		{
			static_assert(N > 0);
			return hash_bytes(lit, N - 1);
		}

		static constexpr sid_t to_sid(const char* s) noexcept
		{
			sid_t h = 1469598103934665603ull;
			for (size_t i = 0; s[i] != '\0'; ++i)
				h = (h ^ static_cast<unsigned char>(s[i])) * 1099511628211ull;
			return h;
		}

		template <class S> static constexpr auto to_sid(const S& s) noexcept -> decltype(s.data(), s.size(), sid_t{})
		{
			return hash_bytes(s.data(), static_cast<size_t>(s.size()));
		}
	};

	constexpr sid_t operator"" _hs(const char* str, size_t len) noexcept
	{
		return hashing_t::hash_bytes(str, len);
	}

#define TO_SID(X)  ::sfg::hashing_t::to_sid((X))
#define TO_SIDC(X) (X##_hs)
}
