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

#include <sfg/common/size_definitions.hpp>

namespace sfg
{

	template <typename T> class bitmask_t
	{
	public:
		bitmask_t()	 = default;
		~bitmask_t() = default;

		bitmask_t(T m) : _mask(m) {};

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

		inline bool operator==(const bitmask_t<T>& other) const
		{
			return _mask == other._mask;
		}

	private:
		T _mask = 0;
	};

	typedef bitmask_t<u8>  bitmask8;
	typedef bitmask_t<u16> bitmask16;
	typedef bitmask_t<u32> bitmask32;
}
