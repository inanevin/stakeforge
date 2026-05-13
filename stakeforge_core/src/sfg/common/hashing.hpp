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
		static constexpr u64 FNV_1A_64_OFFSET = 14695981039346656037ull;
		static constexpr u64 FNV_1A_64_PRIME  = 1099511628211ull;

		static inline u64 hash_fnv_1a64(u64 hash, const void* ptr, size_t len) noexcept
		{
			const u8* bytes = static_cast<const u8*>(ptr);
			for (size_t i = 0; i < len; ++i)
				hash = (hash ^ bytes[i]) * FNV_1A_64_PRIME;
			return hash;
		}

		static inline u64 hash_fnv_1a64(const void* ptr, size_t len) noexcept
		{
			return hash_fnv_1a64(FNV_1A_64_OFFSET, ptr, len);
		}

		static constexpr u64 hash_fnv_1a64(const char* str) noexcept
		{
			u64 hash = FNV_1A_64_OFFSET;
			for (size_t i = 0; str[i] != '\0'; ++i)
				hash = (hash ^ static_cast<u8>(str[i])) * FNV_1A_64_PRIME;
			return hash;
		}

		template <typename T> static inline u64 hash_fnv_1a64_value(u64 hash, const T& value) noexcept
		{
			return hash_fnv_1a64(hash, &value, sizeof(value));
		}

		template <typename... Args> static inline u64 hash_fnv_1a64_values(u64 hash, const Args&... args) noexcept
		{
			((hash = hash_fnv_1a64_value(hash, args)), ...);
			return hash;
		}

		template <typename... Args> static inline u64 hash_fnv_1a64_bytes_and_values(const void* ptr, size_t len, const Args&... args) noexcept
		{
			return hash_fnv_1a64_values(hash_fnv_1a64(ptr, len), args...);
		}

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
