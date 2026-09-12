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
	class ostream_t
	{
	public:
		ostream_t() = default;
		~ostream_t()
		{
			destroy();
		}

		ostream_t(const ostream_t&)			   = delete;
		ostream_t& operator=(const ostream_t&) = delete;

		ostream_t(ostream_t&& other) noexcept
		{
			move_from(other);
		}

		ostream_t& operator=(ostream_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			destroy();
			move_from(other);
			return *this;
		}

		void create(size_t size);
		void destroy();
		void write_to_ofstream(std::ofstream& stream);
		void write_raw_endian_safe(const u8* ptr, size_t size);
		void write_raw(const u8* ptr, size_t size);

		template <typename T> std::enable_if_t<std::is_trivially_copyable_v<T>, void> write(const T& t)
		{
			if (_data == nullptr)
				create(sizeof(T));

			const u8* ptr  = reinterpret_cast<const u8*>(&t);
			size_t	  size = sizeof(T);

			check_grow(size);
			SFG_MEMCPY(&_data[_current_size], ptr, size);
			_current_size += size;
		}

		inline size_t get_size() const
		{
			return _current_size;
		}

		inline u8* get_raw() const
		{
			return _data;
		}

		inline void shrink(size_t size)
		{
			SFG_ASSERT(size <= _total_size);
			_current_size = size;
		}

		inline void set(size_t pad, size_t sz, u8 val)
		{
			SFG_ASSERT(pad <= _total_size && sz <= _total_size - pad);
			SFG_MEMSET(_data + pad, val, sz);
		}

		inline span_t<u8> evict()
		{
			span_t<u8> sp = {_data, _total_size};
			_data		  = nullptr;
			_current_size = _total_size = 0;
			return sp;
		}

	private:
		void check_grow(size_t sz);
		void move_from(ostream_t& other) noexcept;

	private:
		u8*	   _data		 = nullptr;
		size_t _current_size = 0;
		size_t _total_size	 = 0;
	};

	// arithmetic
	template <typename T> std::enable_if_t<std::is_arithmetic_v<std::remove_reference_t<T>>, ostream_t&> operator<<(ostream_t& stream, T&& val)
	{
		using U = std::remove_cv_t<std::remove_reference_t<T>>;
		U copy	= static_cast<U>(val);
		if (endianness::should_swap())
			endianness::swap_endian(copy);
		stream.write(copy);
		return stream;
	}

	// string
	inline ostream_t& operator<<(ostream_t& stream, const string_t& val)
	{
		const u32 sz = static_cast<u32>(val.size());
		stream << sz;
		stream.write_raw(reinterpret_cast<const u8*>(val.data()), val.size());
		return stream;
	}

	// enums
	template <typename T> std::enable_if_t<std::is_enum_v<std::remove_reference_t<T>>, ostream_t&> operator<<(ostream_t& stream, T&& val)
	{
		const u8 u = static_cast<u8>(val);
		stream << u;
		return stream;
	}

	// classes with serialize()
	template <typename T> auto operator<<(ostream_t& stream, T&& val) -> decltype(val.serialize(stream), stream)
	{
		val.serialize(stream);
		return stream;
	}

}
