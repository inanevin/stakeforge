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

#include "raw_stream.hpp"
#include "ostream.hpp"
#include "istream.hpp"
#include <sfg/memory/memory.hpp>

namespace sfg
{
	void raw_stream_t::create(u8* data, size_t size)
	{
		destroy();
		if (size == 0)
			return;

		SFG_ASSERT(data != nullptr);
		_data = {new u8[size], size};
		SFG_MEMCPY(_data.data, data, size);
	}

	void raw_stream_t::create(ostream_t& stream)
	{
		destroy();
		if (stream.get_size() == 0)
			return;

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
		stream << sz;
		if (sz != 0)
			stream.write_raw(_data.data, _data.size);
	}

	void raw_stream_t::deserialize(istream_t& stream)
	{
		u32 size = 0;
		stream >> size;
		destroy();
		if (size != 0)
		{
			const size_t sz = static_cast<size_t>(size);
			_data			= {new u8[sz], sz};
			stream.read_to_raw(_data.data, _data.size);
		}
	}

}
