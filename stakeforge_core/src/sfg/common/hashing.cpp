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

#include "hashing.hpp"

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

namespace sfg
{
	namespace
	{
		u64 make_guid64_seed()
		{
			static std::atomic<u64> counter = 0;

			std::random_device rd;
			const u64		   r0		 = (static_cast<u64>(rd()) << 32) | static_cast<u64>(rd());
			const u64		   r1		 = (static_cast<u64>(rd()) << 32) | static_cast<u64>(rd());
			const u64		   now		 = static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
			const u64		   thread_id = static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
			const u64		   seq		 = counter.fetch_add(1, std::memory_order_relaxed);

			return hashing_t::hash_u64_combine(hashing_t::hash_u64(&r0, sizeof(r0)), r1, now, thread_id, seq);
		}
	}

	u64 hashing_t::generate_guid64()
	{
		thread_local std::mt19937_64 rng(make_guid64_seed());

		u64 guid = rng();
		while (guid == 0)
			guid = rng();

		return guid;
	}
}
