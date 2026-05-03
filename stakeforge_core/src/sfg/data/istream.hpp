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

#pragma once

#include "string.hpp"
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
