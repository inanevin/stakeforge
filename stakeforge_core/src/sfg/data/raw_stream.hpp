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

#pragma once

#include "span.hpp"
#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class ostream_t;
	class istream_t;

	class raw_stream_t
	{
	public:
		raw_stream_t() : _data({}) {};
		~raw_stream_t()
		{
			destroy();
		}

		raw_stream_t(const raw_stream_t& other)			   = delete;
		raw_stream_t& operator=(const raw_stream_t& other) = delete;

		raw_stream_t(raw_stream_t&& other) noexcept
		{
			move_from(other);
		}

		raw_stream_t& operator=(raw_stream_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			destroy();
			move_from(other);
			return *this;
		}

		void create(ostream_t& stream);
		void create(u8* data, size_t size);
		void destroy();
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline span_t<u8> get_span()
		{
			return _data;
		}

		inline u8* get_raw() const
		{
			return _data.data;
		}

		inline size_t get_size() const
		{
			return _data.size;
		}

		bool is_empty() const
		{
			return _data.size == 0;
		}

	private:
		void move_from(raw_stream_t& other) noexcept
		{
			_data		= other._data;
			other._data = {};
		}

	private:
		span_t<u8> _data;
	};

}
