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
	template <typename SIZE_TYPE, typename TAG = void> struct pool_handle_t
	{
		SIZE_TYPE generation = 0;
		SIZE_TYPE index		 = 0;

		bool operator==(const pool_handle_t& other) const
		{
			return generation == other.generation && index == other.index;
		}

		bool is_null() const
		{
			return generation == 0;
		}
	};

	typedef pool_handle_t<u16> pool_handle16;
	typedef pool_handle_t<u32> pool_handle32;

}
