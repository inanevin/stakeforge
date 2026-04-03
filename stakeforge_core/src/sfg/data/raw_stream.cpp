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

#include "raw_stream.hpp"
#include "ostream.hpp"
#include "istream.hpp"
#include "memory/memory.hpp"

namespace sfg
{
	void raw_stream_t::create(u8* data, size_t size)
	{
		destroy();
		_data = {new u8[size], size};
		SFG_MEMCPY(_data.data, data, size);
	}

	void raw_stream_t::create(ostream_t& stream)
	{
		destroy();
		_data = {new u8[stream.get_size()], stream.get_size()};
		SFG_MEMCPY(_data.data, stream.get_raw(), stream.get_size());
	}

	void raw_stream_t::destroy()
	{
		if (is_empty())
			return;
		delete[] _data.data;
		_data = {};
	}

	void raw_stream_t::serialize(ostream_t& stream) const
	{
		const u32 sz = static_cast<u32>(_data.size);
		stream.write(sz);
		if (sz != 0)
			stream.write_raw(_data.data, _data.size);
	}

	void raw_stream_t::deserialize(istream_t& stream)
	{
		u32 size = 0;
		stream.read(size);
		if (size != 0)
		{
			const size_t sz = static_cast<size_t>(size);
			destroy();
			_data = {new u8[sz], sz};
			stream.read_to_raw(_data.data, _data.size);
		}
	}

}
