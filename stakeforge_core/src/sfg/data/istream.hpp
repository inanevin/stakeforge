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

#include "string.hpp"
#include "span.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/serialization/endianness.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/io/assert.hpp>
#include <type_traits>

namespace sfg
{
	class istream_t
	{
	public:
		istream_t() = default;

		istream_t(u8* data, size_t size)
		{
			open(data, size);
		}

		~istream_t()
		{
			destroy();
		}

		istream_t(const istream_t&)			   = delete;
		istream_t& operator=(const istream_t&) = delete;

		istream_t(istream_t&& other) noexcept
		{
			move_from(other);
		}

		istream_t& operator=(istream_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			destroy();
			move_from(other);
			return *this;
		}

		void open(u8* data, size_t size);
		void close();
		void create(u8* data, size_t size);
		void destroy();
		void read_to_raw_endian_safe(void* ptr, size_t size);
		void read_from_ifstream(std::ifstream& stream);
		void read_to_raw(u8* ptr, size_t size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		template <typename T> std::enable_if_t<std::is_trivially_copyable_v<T>, void> read(T& t)
		{
			SFG_ASSERT(_data != nullptr);
			SFG_ASSERT(sizeof(T) <= _size - _index);
			SFG_MEMCPY(reinterpret_cast<u8*>(&t), &_data[_index], sizeof(T));
			_index += sizeof(T);
		}

		inline void skip_by(size_t size)
		{
			SFG_ASSERT(size <= _size - _index);
			_index += size;
		}

		inline void seek(size_t ind)
		{
			SFG_ASSERT(ind <= _size);
			_index = ind;
		}

		inline size_t get_size() const
		{
			return _size;
		}

		inline bool empty() const
		{
			return _size == 0;
		}

		inline u8* get_raw() const
		{
			return _data;
		}

		inline u8* get_data_current()
		{
			SFG_ASSERT(_index <= _size);
			return &_data[_index];
		}

		inline void shrink(size_t size)
		{
			SFG_ASSERT(size <= _size);
			_size = size;
			if (_index > _size)
				_index = _size;
		}

		inline size_t tellg() const
		{
			return _index;
		}

		inline bool is_eof() const
		{
			return _index >= _size;
		}

		inline span_t<u8> evict()
		{
			span_t<u8> sp = {_data, _size};
			_data		  = nullptr;
			_owns		  = false;
			_index = _size = 0;
			return sp;
		}

	private:
		void move_from(istream_t& other) noexcept;

	private:
		u8*	   _data  = nullptr;
		size_t _index = 0;
		size_t _size  = 0;
		bool   _owns  = false;
	};

	template <typename T> std::enable_if_t<std::is_arithmetic_v<std::remove_reference_t<T>>, istream_t&> operator>>(istream_t& stream, T& val)
	{
		stream.read(val);
		if (endianness::should_swap())
			endianness::swap_endian(val);
		return stream;
	}

	template <typename T> std::enable_if_t<std::is_enum_v<std::remove_reference_t<T>>, istream_t&> operator>>(istream_t& stream, T& val)
	{
		u8 u8 = 0;
		stream >> u8;
		val = static_cast<std::remove_reference_t<T>>(u8);
		return stream;
	}

	inline istream_t& operator>>(istream_t& stream, string_t& val)
	{
		u32 sz = 0;
		stream >> sz;
		val = string_t(reinterpret_cast<char*>(stream.get_data_current()), sz);
		stream.skip_by(sz);
		return stream;
	}

	template <typename T> auto operator>>(istream_t& stream, T& val) -> decltype(val.deserialize(stream), stream)
	{
		val.deserialize(stream);
		return stream;
	}

	template <typename T> istream_t& operator>>(istream_t& stream, T&&) = delete;

}
