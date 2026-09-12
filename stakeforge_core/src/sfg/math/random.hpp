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
#include <random>

namespace sfg
{
	namespace random_t
	{
		std::mt19937& rng();
		void		  seed_rng(uint64_t seed);

		// Uniform real in [0, 1).
		f32 random_01();

		// [min_inclusive, max_inclusive].
		int random_int(int min_inclusive, int max_inclusive);

		u32 next_u32(u32& state);
		f32 random_01(u32& state);
		f32 random_range(u32& state, f32 minimum, f32 maximum);
	}

}
