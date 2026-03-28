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

#include "bump_text_allocator.hpp"
#include "memory/memory.hpp"
#include "data/char_util.hpp"

#include <cstdarg>
#include <cstring>

namespace SFG
{
	bump_text_allocator::~bump_text_allocator()
	{
		uninit();
	}

	void bump_text_allocator::init(size_t capacity_bytes)
	{
		uninit();

		if (capacity_bytes == 0)
			return;

		_raw  = new (std::nothrow) char[capacity_bytes];
		_cap  = _raw ? capacity_bytes : 0;
		_head = 0;

		_cur_start = _cur = _cur_end = nullptr;

		if (_raw)
			SFG_MEMSET(_raw, 0, _cap);
	}

	void bump_text_allocator::uninit()
	{
		delete[] _raw;
		_raw	   = nullptr;
		_cap	   = 0;
		_head	   = 0;
		_cur_start = _cur = _cur_end = nullptr;
	}

	void bump_text_allocator::reset()
	{
		_head	   = 0;
		_cur_start = _cur = _cur_end = nullptr;
	}

	const char* bump_text_allocator::allocate_reserve(size_t reserve_bytes_including_null)
	{
		if (!_raw || reserve_bytes_including_null == 0)
			return nullptr;

		if (_head + reserve_bytes_including_null > _cap)
			return nullptr;

		_cur_start = _raw + _head;
		_cur	   = _cur_start;
		_cur_end   = _cur_start + reserve_bytes_including_null;

		// valid immediately
		*_cur = '\0';

		_head += reserve_bytes_including_null;
		return _cur_start;
	}

	const char* bump_text_allocator::allocate(const char* initial_text, size_t reserve_extra)
	{
		if (!initial_text)
			initial_text = "";

		const size_t init_len = std::strlen(initial_text);
		const size_t reserve  = init_len + 1 + reserve_extra;

		const char* start = allocate_reserve(reserve);
		if (!start)
			return nullptr;

		if (init_len > 0)
		{
			if (!append(string_view(initial_text, init_len)))
				return nullptr;
		}

		null_terminate_in_place();
		return start;
	}

	const char* bump_text_allocator::terminate()
	{
		if (!_cur_start)
			return nullptr;

		null_terminate_in_place();
		return _cur_start;
	}

	const char* bump_text_allocator::current_c_str() const
	{
		return _cur_start ? _cur_start : "";
	}

	size_t bump_text_allocator::remaining() const
	{
		if (!_cur_start)
			return 0;

		return (_cur_end > _cur) ? size_t(_cur_end - _cur) : 0;
	}

	bool bump_text_allocator::append(string_view s)
	{
		if (!_cur_start)
			return false;

		return char_util::append(_cur, _cur_end, s.data(), s.size());
	}

	bool bump_text_allocator::append(const char* s)
	{
		if (!_cur_start)
			return false;

		return char_util::append(_cur, _cur_end, s);
	}

	bool bump_text_allocator::append(char c)
	{
		if (!_cur_start)
			return false;

		return char_util::append_char(_cur, _cur_end, c);
	}

	bool bump_text_allocator::append(i32 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i32(_cur, _cur_end, v);
	}
	bool bump_text_allocator::append(u32 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u32(_cur, _cur_end, v);
	}
	bool bump_text_allocator::append(i64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i64(_cur, _cur_end, v);
	}
	bool bump_text_allocator::append(u64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u64(_cur, _cur_end, v);
	}

	bool bump_text_allocator::append_i64(i64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i64(_cur, _cur_end, v);
	}

	bool bump_text_allocator::append_u64(u64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u64(_cur, _cur_end, v);
	}

	bool bump_text_allocator::append(double v, int precision)
	{
		if (!_cur_start)
			return false;

		return char_util::append_double(_cur, _cur_end, v, precision);
	}

	bool bump_text_allocator::appendf(const char* fmt, ...)
	{
		if (!_cur_start)
			return false;

		va_list args;
		va_start(args, fmt);
		bool ok = char_util::appendf_va(_cur, _cur_end, fmt, args);
		va_end(args);
		return ok;
	}

	bool bump_text_allocator::ensure_space(size_t bytes_needed_including_null) const
	{
		return remaining() >= bytes_needed_including_null;
	}

	void bump_text_allocator::null_terminate_in_place()
	{
		if (!_cur_start)
			return;

		if (_cur < _cur_end)
			*_cur = '\0';
		else
			*(_cur_end - 1) = '\0';
	}
}
