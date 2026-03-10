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

	template <typename T> class bitmask
	{
	public:
		bitmask()  = default;
		~bitmask() = default;

		bitmask(T m) : _mask(m){};

		inline bool is_set(T m) const
		{
			return (_mask & m) != 0;
		}

		inline bool is_all_set(T bits) const
		{
			return (_mask & bits) == bits;
		}

		inline void set(T m)
		{
			_mask |= m;
		}

		inline void set(T m, bool isSet)
		{
			if (isSet)
				_mask |= m;
			else
				_mask &= ~m;
		}

		inline void remove(T m)
		{
			_mask &= ~m;
		}

		inline T value() const
		{
			return _mask;
		}

		inline bool operator==(const bitmask<T>& other) const
		{
			return _mask == other._mask;
		}

	private:
		T _mask = 0;
	};

	typedef bitmask<u8>	 bitmask8;
	typedef bitmask<u16> bitmask16;
	typedef bitmask<u32> bitmask32;
}
