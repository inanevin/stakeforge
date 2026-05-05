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
