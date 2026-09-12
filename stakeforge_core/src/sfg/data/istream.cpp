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
