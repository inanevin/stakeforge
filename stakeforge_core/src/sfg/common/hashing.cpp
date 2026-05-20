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
