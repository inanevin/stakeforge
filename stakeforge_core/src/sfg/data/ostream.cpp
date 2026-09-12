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
