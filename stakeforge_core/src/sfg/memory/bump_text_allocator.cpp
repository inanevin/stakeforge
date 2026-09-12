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

#include "bump_text_allocator.hpp"
#include <sfg/memory/memory.hpp>
#include <sfg/data/char_util.hpp>

namespace sfg
{
	bump_text_allocator_t::~bump_text_allocator_t()
	{
		uninit();
	}

	void bump_text_allocator_t::init(size_t capacity_bytes)
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

	void bump_text_allocator_t::uninit()
	{
		delete[] _raw;
		_raw	   = nullptr;
		_cap	   = 0;
		_head	   = 0;
		_cur_start = _cur = _cur_end = nullptr;
	}

	void bump_text_allocator_t::reset()
	{
		_head	   = 0;
		_cur_start = _cur = _cur_end = nullptr;
	}

	const char* bump_text_allocator_t::allocate_reserve(size_t reserve_bytes_including_null)
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

	const char* bump_text_allocator_t::allocate(const char* initial_text, size_t reserve_extra)
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
			if (!append(string_view_t(initial_text, init_len)))
				return nullptr;
		}

		null_terminate_in_place();
		return start;
	}

	const char* bump_text_allocator_t::terminate()
	{
		if (!_cur_start)
			return nullptr;

		null_terminate_in_place();
		return _cur_start;
	}

	const char* bump_text_allocator_t::current_c_str() const
	{
		return _cur_start ? _cur_start : "";
	}

	size_t bump_text_allocator_t::remaining() const
	{
		if (!_cur_start)
			return 0;

		return (_cur_end > _cur) ? size_t(_cur_end - _cur) : 0;
	}

	bool bump_text_allocator_t::append(string_view_t s)
	{
		if (!_cur_start)
			return false;

		return char_util::append(_cur, _cur_end, s.data(), s.size());
	}

	bool bump_text_allocator_t::append(const char* s)
	{
		if (!_cur_start)
			return false;

		return char_util::append(_cur, _cur_end, s);
	}

	bool bump_text_allocator_t::append(char c)
	{
		if (!_cur_start)
			return false;

		return char_util::append_char(_cur, _cur_end, c);
	}

	bool bump_text_allocator_t::append(i32 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i32(_cur, _cur_end, v);
	}
	bool bump_text_allocator_t::append(u32 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u32(_cur, _cur_end, v);
	}
	bool bump_text_allocator_t::append(i64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i64(_cur, _cur_end, v);
	}
	bool bump_text_allocator_t::append(u64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u64(_cur, _cur_end, v);
	}

	bool bump_text_allocator_t::append_i64(i64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_i64(_cur, _cur_end, v);
	}

	bool bump_text_allocator_t::append_u64(u64 v)
	{
		if (!_cur_start)
			return false;

		return char_util::append_u64(_cur, _cur_end, v);
	}

	bool bump_text_allocator_t::append(double v, int precision)
	{
		if (!_cur_start)
			return false;

		return char_util::append_double(_cur, _cur_end, v, precision);
	}

	bool bump_text_allocator_t::appendf(const char* fmt, ...)
	{
		if (!_cur_start)
			return false;

		va_list args;
		va_start(args, fmt);
		bool ok = char_util::appendf_va(_cur, _cur_end, fmt, args);
		va_end(args);
		return ok;
	}

	bool bump_text_allocator_t::ensure_space(size_t bytes_needed_including_null) const
	{
		return remaining() >= bytes_needed_including_null;
	}

	void bump_text_allocator_t::null_terminate_in_place()
	{
		if (!_cur_start)
			return;

		if (_cur < _cur_end)
			*_cur = '\0';
		else
			*(_cur_end - 1) = '\0';
	}
}
