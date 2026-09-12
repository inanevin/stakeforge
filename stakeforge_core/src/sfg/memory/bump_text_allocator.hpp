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

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class bump_text_allocator_t
	{
	public:
		struct string_view_t
		{
			const char* ptr;
			size_t		sz;

			inline bool empty()
			{
				return sz == 0;
			}

			inline const char* data()
			{
				return ptr;
			}

			inline size_t size()
			{
				return sz;
			}
		};

		bump_text_allocator_t() = default;
		~bump_text_allocator_t();

		bump_text_allocator_t(const bump_text_allocator_t&)			   = delete;
		bump_text_allocator_t& operator=(const bump_text_allocator_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(size_t capacity_bytes);
		void uninit();
		void reset();

		// -----------------------------------------------------------------------------
		// string building
		// -----------------------------------------------------------------------------

		const char* allocate_reserve(size_t reserve_bytes_including_null);
		const char* allocate(const char* initial_text, size_t reserve_extra = 0);
		const char* terminate();
		const char* current_c_str() const;
		size_t		remaining() const;

		// -----------------------------------------------------------------------------
		// append utilities
		// -----------------------------------------------------------------------------

		bool append(string_view_t s);
		bool append(const char* s);
		bool append(char c);
		bool append(i32 v);
		bool append(u32 v);
		bool append(i64 v);
		bool append(u64 v);
		bool append(double v, int precision = 3);
		bool appendf(const char* fmt, ...);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		size_t capacity() const
		{
			return _cap;
		}
		size_t head() const
		{
			return _head;
		}
		const char* raw() const
		{
			return _raw;
		}

	private:
		bool append_i64(i64 v);
		bool append_u64(u64 v);
		bool ensure_space(size_t bytes_needed_including_null) const;
		void null_terminate_in_place();

	private:
		char*  _raw	 = nullptr;
		size_t _cap	 = 0;
		size_t _head = 0;

		// Active string state
		char* _cur_start = nullptr;
		char* _cur		 = nullptr;
		char* _cur_end	 = nullptr; // one-past last byte owned by active reservation
	};
}
