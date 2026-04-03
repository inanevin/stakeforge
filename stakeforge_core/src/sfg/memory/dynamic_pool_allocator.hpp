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

#include <type_traits>

#include "common/size_definitions.hpp"
#include "io/assert.hpp"
#include "memory.hpp"

namespace sfg
{

	template <typename T, typename SIZE_TYPE = u32> class dynamic_pool_allocator_t
	{

	private:
		static_assert(std::is_trivial_v<T> && std::is_integral_v<SIZE_TYPE> && std::is_unsigned_v<SIZE_TYPE>);

	public:
		dynamic_pool_allocator_t()										= default;
		dynamic_pool_allocator_t(const dynamic_pool_allocator_t& other) = delete;

		~dynamic_pool_allocator_t()
		{
			clear();
		}

		dynamic_pool_allocator_t& operator=(dynamic_pool_allocator_t& other)
		{
			_data				  = other._data;
			_free_list			  = other._free_list;
			_free_list_head		  = other._free_list_head;
			_size				  = other._size;
			_capacity			  = other._capacity;
			other._data			  = nullptr;
			other._free_list	  = nullptr;
			other._free_list_head = 0;
			other._size			  = 0;
			other._capacity		  = 0;
			return *this;
		}

		inline SIZE_TYPE add()
		{
			if (_free_list_head != 0)
			{
				const SIZE_TYPE idx = _free_list[_free_list_head - 1];
				--_free_list_head;
				return idx;
			}

			check_grow();

			const SIZE_TYPE idx = _size;
			_size++;
			return idx;
		}

		inline const T& get(SIZE_TYPE id) const
		{
			SFG_ASSERT(id < _capacity && id < _size);
			return _data[id];
		}

		inline T& get(SIZE_TYPE id)
		{
			SFG_ASSERT(id < _capacity && id < _size);
			return _data[id];
		}

		inline bool remove(SIZE_TYPE id)
		{
			SFG_ASSERT(id < _capacity && id < _size);
			_free_list[_free_list_head] = id;
			_free_list_head++;
			return true;
		}

		inline void reserve(SIZE_TYPE desired_cap)
		{
			if (desired_cap <= _capacity)
				return;

			check_grow(desired_cap);
		}

		inline SIZE_TYPE size() const
		{
			return _size;
		}

		inline SIZE_TYPE capacity() const
		{
			return _capacity;
		}

		inline void resize(SIZE_TYPE s)
		{
			if (s == _size)
				return;

			const SIZE_TYPE old_size = _size;
			if (s > _capacity)
				check_grow(s);

			_size = s;

			if (_size < old_size)
			{
				SIZE_TYPE new_free_list_head = 0;
				for (SIZE_TYPE i = 0; i < _free_list_head; i++)
				{
					if (_free_list[i] < _size)
					{
						_free_list[new_free_list_head] = _free_list[i];
						new_free_list_head++;
					}
				}

				_free_list_head = new_free_list_head;
			}
		}

		inline void clear()
		{
			if (_data)
			{
				delete[] _data;
				delete[] _free_list;
			}

			_size = _capacity = 0;
			_free_list_head	  = 0;
			_data			  = nullptr;
			_free_list		  = nullptr;
		}

	private:
		inline void check_grow(SIZE_TYPE desired_cap = 0)
		{
			if (_size < _capacity && desired_cap == 0)
				return;

			const SIZE_TYPE new_cap		  = desired_cap != 0 ? desired_cap : (_capacity == 0 ? 4 : (_capacity * 2));
			T*				new_data	  = new T[new_cap];
			SIZE_TYPE*		new_free_list = new SIZE_TYPE[new_cap];

			if (_data)
			{
				if (_size != 0)
					SFG_MEMCPY(new_data, _data, sizeof(T) * _size);

				if (_free_list_head != 0)
					SFG_MEMCPY(new_free_list, _free_list, sizeof(SIZE_TYPE) * _free_list_head);

				delete[] _data;
				delete[] _free_list;
				_data	   = nullptr;
				_free_list = nullptr;
			}

			_data	   = new_data;
			_free_list = new_free_list;
			_capacity  = new_cap;
		}

	private:
		T*		   _data		   = nullptr;
		SIZE_TYPE* _free_list	   = nullptr;
		SIZE_TYPE  _free_list_head = 0;
		SIZE_TYPE  _size		   = 0;
		SIZE_TYPE  _capacity	   = 0;
	};

	template <typename T, typename SIZE_TYPE = u32> using dynamic_pool_t = dynamic_pool_allocator_t<T, SIZE_TYPE>;

}
