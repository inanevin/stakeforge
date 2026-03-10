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

#ifdef SFG_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 28251)
#pragma warning(disable : 6001)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <bit>
#include <array>

namespace SFG
{
	class endianness
	{
	public:
		/// <summary>
		///
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="val"></param>
		/// <param name=""></param>
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

		/// <summary>
		///
		/// </summary>
		/// <returns></returns>
		static inline bool should_swap()
		{
			return std::endian::native == std::endian::big;
		}
	};

}

#ifdef SFG_COMPILER_MSVC
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif