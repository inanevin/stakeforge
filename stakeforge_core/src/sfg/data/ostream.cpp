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

#include "ostream.hpp"
#include <sfg/data/vector.hpp>
#include <fstream>

namespace sfg
{
	void ostream_t::create(size_t size)
	{
		destroy();
		if (size == 0)
			size = 1;

		_data		  = new u8[size];
		_total_size	  = size;
		_current_size = 0;
	}

	void ostream_t::destroy()
	{
		delete[] _data;

		_current_size = 0;
		_total_size	  = 0;
		_data		  = nullptr;
	}

	void ostream_t::write_raw_endian_safe(const u8* ptr, size_t size)
	{
		if (size == 0)
			return;

		SFG_ASSERT(ptr != nullptr);

		if (_data == nullptr)
			create(size);

		check_grow(size);

		if (endianness::should_swap())
		{
			vector_t<u8> v;
			v.insert(v.end(), ptr, (ptr) + size);

			vector_t<u8> v2;
			v2.resize(v.size());

			const size_t sz = v.size();
			for (size_t i = 0; i < sz; i++)
			{
				v2[i] = v[sz - i - 1];
			}

			SFG_MEMCPY(&_data[_current_size], v2.data(), size);

			v.clear();
			v2.clear();
		}
		else
			SFG_MEMCPY(&_data[_current_size], ptr, size);

		_current_size += size;
	}

	void ostream_t::write_raw(const u8* ptr, size_t size)
	{
		if (size == 0)
			return;

		SFG_ASSERT(ptr != nullptr);

		if (_data == nullptr)
			create(size);

		check_grow(size);
		SFG_MEMCPY(&_data[_current_size], ptr, size);
		_current_size += size;
	}

	void ostream_t::check_grow(size_t sz)
	{
		if (_current_size + sz > _total_size)
		{
			const size_t required = _current_size + sz;
			size_t		 new_size = _total_size == 0 ? 1 : _total_size;
			while (new_size < required)
				new_size *= 2;

			_total_size = new_size;
			u8* newData = new u8[_total_size];
			SFG_MEMCPY(newData, _data, _current_size);
			delete[] _data;
			_data = newData;
		}
	}

	void ostream_t::move_from(ostream_t& other) noexcept
	{
		_data		  = other._data;
		_current_size = other._current_size;
		_total_size	  = other._total_size;

		other._data			= nullptr;
		other._current_size = 0;
		other._total_size	= 0;
	}

	void ostream_t::write_to_ofstream(std::ofstream& stream)
	{
		stream.write((char*)_data, _current_size);
	}

}
