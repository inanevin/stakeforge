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
#include "size_definitions.hpp"

namespace sfg
{
	constexpr sid_t operator"" _hs(const char* str, size_t len) noexcept;

	// FNV-1a: Glenn Fowler, Landon Curt Noll, Kiem-Phong Vo - https://github.com/lcn2/fnv/blob/master/hash_64a.c
	// SID hashing uses a different offset basis.
	class hashing_t
	{
	public:
		static inline u64 hash_u64(const void* ptr, size_t len) noexcept
		{
			return hash_u64(FNV_OFFSET, ptr, len);
		}

		static inline u64 hash_u64(u64 seed, const void* ptr, size_t len) noexcept
		{
			const u8* bytes = static_cast<const u8*>(ptr);
			for (size_t i = 0; i < len; ++i)
				seed = (seed ^ bytes[i]) * FNV_PRIME;
			return seed;
		}

		static constexpr u64 hash_u64(const char* str) noexcept
		{
			u64 hash = FNV_OFFSET;
			for (size_t i = 0; str[i] != '\0'; ++i)
				hash = (hash ^ static_cast<u8>(str[i])) * FNV_PRIME;
			return hash;
		}

		template <typename... Args> static inline u64 hash_u64_combine(u64 seed, const Args&... args) noexcept
		{
			((seed = hash_u64(seed, &args, sizeof(args))), ...);
			return seed;
		}

		template <size_t N> static consteval sid_t to_sid(const char (&lit)[N]) noexcept
		{
			static_assert(N > 0);
			return hash_sid_bytes(lit, N - 1);
		}

		static constexpr sid_t to_sid(const char* s) noexcept
		{
			sid_t h = SID_OFFSET;
			for (size_t i = 0; s[i] != '\0'; ++i)
				h = (h ^ static_cast<u8>(s[i])) * FNV_PRIME;
			return h;
		}

		template <class S> static constexpr auto to_sid(const S& s) noexcept -> decltype(s.data(), s.size(), sid_t{})
		{
			return hash_sid_bytes(s.data(), static_cast<size_t>(s.size()));
		}

		static u64 generate_guid64();

	private:
		friend constexpr sid_t operator"" _hs(const char* str, size_t len) noexcept;

		static constexpr u64 FNV_OFFSET = 14695981039346656037ull;
		static constexpr u64 FNV_PRIME	= 1099511628211ull;
		static constexpr u64 SID_OFFSET = 1469598103934665603ull;

		static constexpr sid_t hash_sid_bytes(const char* str, size_t len) noexcept
		{
			sid_t h = SID_OFFSET;
			for (size_t i = 0; i < len; ++i)
				h = (h ^ static_cast<u8>(str[i])) * FNV_PRIME;
			return h;
		}
	};

	constexpr sid_t operator"" _hs(const char* str, size_t len) noexcept
	{
		return hashing_t::hash_sid_bytes(str, len);
	}

#define TO_SID(X)  ::sfg::hashing_t::to_sid((X))
#define TO_SIDC(X) (X##_hs)
}
