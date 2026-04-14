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

#include "istream.hpp"
#include "vector.hpp"
#include <fstream>

namespace sfg
{
	void istream_t::open(u8* data, size_t size)
	{
		destroy();
		_data  = data;
		_size  = size;
		_index = 0;
		_owns  = false;
	}

	void istream_t::close()
	{
		destroy();
	}
	void istream_t::create(u8* data, size_t size)
	{
		destroy();
		if (size == 0)
			return;

		_data = new u8[size];
		_owns = true;

		if (data != nullptr)
			SFG_MEMCPY(_data, data, size);

		_index = 0;
		_size  = size;
	}

	void istream_t::destroy()
	{
		if (_owns)
			delete[] _data;

		_index = 0;
		_size  = 0;
		_data  = nullptr;
		_owns  = false;
	}

	void istream_t::read_from_ifstream(std::ifstream& stream)
	{
		SFG_ASSERT(_data != nullptr);
		stream.read((char*)_data, _size);
	}

	void istream_t::read_to_raw_endian_safe(void* ptr, size_t size)
	{
		SFG_ASSERT(ptr != nullptr);
		SFG_ASSERT(_data != nullptr);
		SFG_ASSERT(size <= _size - _index);

		if (endianness::should_swap())
		{
			u8*			 data = &_data[_index];
			vector_t<u8> v;
			v.insert(v.end(), data, data + size);

			vector_t<u8> v2;
			v2.resize(v.size());

			const size_t sz = v.size();
			for (size_t i = 0; i < sz; i++)
			{
				v2[i] = v[sz - i - 1];
			}

			SFG_MEMCPY(ptr, v2.data(), size);

			v.clear();
			v2.clear();
		}
		else
			SFG_MEMCPY(ptr, &_data[_index], size);

		_index += size;
	}

	void istream_t::read_to_raw(u8* ptr, size_t size)
	{
		SFG_ASSERT(ptr != nullptr);
		SFG_ASSERT(_data != nullptr);
		SFG_ASSERT(size <= _size - _index);
		SFG_MEMCPY(ptr, &_data[_index], size);
		_index += size;
	}

	void istream_t::move_from(istream_t& other) noexcept
	{
		_data  = other._data;
		_index = other._index;
		_size  = other._size;
		_owns  = other._owns;

		other._data	 = nullptr;
		other._index = 0;
		other._size	 = 0;
		other._owns	 = false;
	}

}
