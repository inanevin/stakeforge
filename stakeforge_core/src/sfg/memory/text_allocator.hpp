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
#include <sfg/data/vector.hpp>

namespace sfg
{
	class text_allocator_t
	{

	private:
		struct allocation_t
		{
			char*  ptr	= nullptr;
			size_t size = 0;
		};

	public:
		text_allocator_t() : _head(0) {};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(u32 capacity);
		void uninit();

		// -----------------------------------------------------------------------------
		// memory api
		// -----------------------------------------------------------------------------

		const char* allocate(size_t len);
		const char* allocate(const char* text, size_t len = 0);
		void		deallocate(char* ptr);
		void		deallocate(const char* ptr);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline constexpr size_t get_capacity() const
		{
			return _capacity;
		}

		inline constexpr size_t get_head() const
		{
			return _head;
		}

		inline char* get_raw() const
		{
			return _raw;
		}

		inline void reset()
		{
			_free_list.resize(0);
			_head = 0;
		}

	private:
		void insert_free_allocation_sorted(allocation_t allocation);

		vector_t<allocation_t> _free_list;
		char*				   _raw		 = nullptr;
		u32					   _head	 = 0;
		u32					   _capacity = 0;
	};

}
