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

#include "bump_allocator.hpp"
#include "memory.hpp"

namespace sfg
{
	void bump_allocator_t::init(size_t sz, size_t alignment)
	{
		SFG_ASSERT(sz != 0);
		_size = sz;
		_raw  = SFG_ALIGNED_MALLOC(alignment, sz);
		_owns = 1;
	}

	void bump_allocator_t::init(u8* existing, size_t sz)
	{
		_owns = 0;
		_raw  = existing;
		_size = sz;
	}

	void bump_allocator_t::uninit()
	{
		if (_owns)
		{
			SFG_ALIGNED_FREE(_raw);
		}
		_raw = nullptr;
	}

	void* bump_allocator_t::allocate(size_t size, size_t alignment)
	{
		SFG_ASSERT(size <= _size - _head);

		void*  current_ptr = (void*)((u8*)_raw + _head);
		size_t space	   = _size - _head;

		void* aligned_ptr = std::align(alignment, size, current_ptr, space);
		if (aligned_ptr == nullptr)
			return nullptr;

		_head = _size - space + size;
		return aligned_ptr;
	}
}
