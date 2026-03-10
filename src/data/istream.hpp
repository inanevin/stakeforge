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

#include "common/size_definitions.hpp"
#include "serialization/endianness.hpp"
#include "memory/memory.hpp"
#include "data/string.hpp"
#include "io/assert.hpp"

namespace SFG
{
	class istream
	{
	public:
		istream(){};
		istream(u8* data, size_t size)
		{
			_data = data;
			_size = size;
		}
		void open(u8* data, size_t size);
		void close();
		void create(u8* data, size_t size);
		void destroy();
		void read_from_ifstream(std::ifstream& stream);
		void read_to_raw_endian_safe(void* ptr, size_t size);
		void read_to_raw(u8* ptr, size_t size);

		template <typename T> void read(T& t)
		{
			SFG_MEMCPY(reinterpret_cast<u8*>(&t), &_data[_index], sizeof(T));
			_index += sizeof(T);
		}

		inline void skip_by(size_t size)
		{
			_index += size;
		}

		inline void seek(size_t ind)
		{
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
			return &_data[_index];
		}

		inline void shrink(size_t size)
		{
			_size = size;
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
		u8*	   _data  = nullptr;
		size_t _index = 0;
		size_t _size  = 0;
	};

	template <typename T> std::enable_if_t<std::is_arithmetic_v<std::remove_reference_t<T>>, istream&> operator>>(istream& stream, T& val)
	{
		stream.read(val);
		if (endianness::should_swap())
			endianness::swap_endian(val);
		return stream;
	}

	template <typename T> std::enable_if_t<std::is_enum_v<std::remove_reference_t<T>>, istream&> operator>>(istream& stream, T& val)
	{
		u8 u8 = 0;
		stream >> u8;
		val = static_cast<std::remove_reference_t<T>>(u8);
		return stream;
	}

	inline istream& operator>>(istream& stream, string& val)
	{
		u32 sz = 0;
		stream >> sz;
		val = string(reinterpret_cast<char*>(stream.get_data_current()), sz);
		stream.skip_by(sz);
		return stream;
	}

	template <typename T> auto operator>>(istream& stream, T& val) -> decltype(val.deserialize(stream), stream)
	{
		val.deserialize(stream);
		return stream;
	}

	template <typename T> istream& operator>>(istream& stream, T&&) = delete;

}
