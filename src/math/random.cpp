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

#include "random.hpp"
#include "io/assert.hpp"
#include <cstdint>
#include <chrono>
#include <cassert>

namespace SFG
{
	std::mt19937& random::rng()
	{
		thread_local std::mt19937 engine{[] {
			std::seed_seq seq{static_cast<uint32_t>(std::random_device{}()), static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
			return std::mt19937{seq};
		}()};
		return engine;
	}
	void random::seed_rng(uint64_t seed)
	{
		std::seed_seq seq{static_cast<uint32_t>(seed), static_cast<uint32_t>(seed >> 32), 0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u};
		rng() = std::mt19937{seq};
	}
	f32 random::random_01()
	{
		static thread_local std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
		return dist(rng());
	}

	int random::random_int(int min_inclusive, int max_inclusive)
	{
		SFG_ASSERT(min_inclusive <= max_inclusive);
		std::uniform_int_distribution<int> dist(min_inclusive, max_inclusive);
		return dist(rng());
	}
}
