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

#include "random.hpp"
#include <sfg/io/assert.hpp>
#include <cstdint>
#include <chrono>
#include <cassert>

namespace sfg
{
	std::mt19937& random_t::rng()
	{
		thread_local std::mt19937 engine_t{[] {
			std::seed_seq seq{static_cast<uint32_t>(std::random_device{}()), static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
			return std::mt19937{seq};
		}()};
		return engine_t;
	}

	void random_t::seed_rng(uint64_t seed)
	{
		std::seed_seq seq{static_cast<uint32_t>(seed), static_cast<uint32_t>(seed >> 32), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
		rng() = std::mt19937{seq};
	}

	f32 random_t::random_01()
	{
		static thread_local std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
		return dist(rng());
	}

	int random_t::random_int(int min_inclusive, int max_inclusive)
	{
		SFG_ASSERT(min_inclusive <= max_inclusive);
		std::uniform_int_distribution<int> dist(min_inclusive, max_inclusive);
		return dist(rng());
	}

	// Chris Wellons, lowbias32 - https://nullprogram.com/blog/2018/07/31/
	u32 random_t::next_u32(u32& state)
	{
		state ^= state >> 16;
		state *= 0x7feb352du;
		state ^= state >> 15;
		state *= 0x846ca68bu;
		state ^= state >> 16;

		return state;
	}

	f32 random_t::random_01(u32& state)
	{
		return static_cast<f32>(next_u32(state) >> 8) * (1.0f / 16777216.0f);
	}

	f32 random_t::random_range(u32& state, f32 minimum, f32 maximum)
	{
		return minimum + (maximum - minimum) * random_01(state);
	}
}
