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

#ifdef SFG_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 28251)
#pragma warning(disable : 6001)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <array>
#include <bit>

namespace sfg
{
	namespace endianness
	{
		template <typename T> static inline void swap_endian(T& val, typename std::enable_if<std::is_arithmetic<T>::value, std::nullptr_t>::type = nullptr)
		{
			union U {
				T									val;
				std::array<std::uint8_t, sizeof(T)> raw;
			} src, dst;

			src.val = val;

			const size_t sz = src.raw.size();

			for (size_t i = 0; i < sz; i++)
			{
				dst.raw[i] = src.raw[sz - i - 1];
			}
			val = dst.val;
		}

		static inline bool should_swap()
		{
			return std::endian::native == std::endian::big;
		}
	}

} // namespace sfg

#ifdef SFG_COMPILER_MSVC
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
