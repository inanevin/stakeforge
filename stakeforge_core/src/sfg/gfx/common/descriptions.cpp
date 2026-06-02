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

#include "descriptions.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/math/math.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <cstring>

namespace sfg
{
	namespace
	{
		void set_desc_name(char* dst, size_t capacity, const char* src)
		{
			SFG_ASSERT(src != nullptr);
			if (src == nullptr)
				return;
			const size_t len = std::strlen(src);
			SFG_ASSERT(len < capacity);
			if (len >= capacity)
				return;
			SFG_MEMCPY(dst, src, len + 1);
		}
	}

	void resource_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	void texture_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	bool sampler_desc_t::operator==(const sampler_desc_t& other) const
	{
		return other.anisotropy == anisotropy && other.flags == flags && other.address_u == address_u && other.address_v == address_v && other.address_w == address_w && other.compare == compare && math::almost_equal(min_lod, other.min_lod) &&
			   math::almost_equal(max_lod, other.max_lod) && math::almost_equal(lod_bias, other.lod_bias);
	}

	void sampler_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	void sampler_desc_t::serialize(ostream_t& stream) const
	{
		stream << anisotropy;
		stream << lod_bias;
		stream << min_lod;
		stream << max_lod;
		stream << flags.value();
		stream << address_u;
		stream << address_v;
		stream << address_w;
	}

	void sampler_desc_t::deserialize(istream_t& stream)
	{
		u16 val	   = 0;
		u8	addr_u = 0;
		u8	addr_v = 0;
		u8	addr_w = 0;
		stream >> anisotropy;
		stream >> lod_bias;
		stream >> min_lod;
		stream >> max_lod;
		stream >> val;
		stream >> addr_u;
		stream >> addr_v;
		stream >> addr_w;
		flags	  = val;
		address_u = static_cast<address_mode>(addr_u);
		address_v = static_cast<address_mode>(addr_v);
		address_w = static_cast<address_mode>(addr_w);
	}

}
