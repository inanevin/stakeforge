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

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "common/size_definitions.hpp"
#include "io/assert.hpp"
#include "memory.hpp"

namespace sfg
{

	template <typename T, typename SIZE_TYPE = u32> class dynamic_pool_allocator_t
	{

	private:
		static_assert(std::is_integral_v<SIZE_TYPE>);
		static_assert(std::is_unsigned_v<SIZE_TYPE>);

	public:
		dynamic_pool_allocator_t()										= default;
		dynamic_pool_allocator_t(const dynamic_pool_allocator_t& other) = delete;
		dynamic_pool_allocator_t(dynamic_pool_allocator_t&& other) noexcept
		{
			move_from(other);
		}

		~dynamic_pool_allocator_t()
		{
			clear();
		}

		dynamic_pool_allocator_t& operator=(const dynamic_pool_allocator_t& other) = delete;

		dynamic_pool_allocator_t& operator=(dynamic_pool_allocator_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			clear();
			move_from(other);
			return *this;
		}

		inline SIZE_TYPE add()
		{
			if (_free_list_head != 0)
			{
				const SIZE_TYPE idx = _free_list[_free_list_head - 1];
				--_free_list_head;
				std::construct_at(&_data[idx]);
				_actives[idx] = 1;
				_size++;
				return idx;
			}

			check_grow();

			const SIZE_TYPE idx = _head;
			_head++;
			_size++;
			std::construct_at(&_data[idx]);
			_actives[idx] = 1;
			return idx;
		}

		inline const T& get(SIZE_TYPE id) const
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			return _data[id];
		}

		inline T& get(SIZE_TYPE id)
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			return _data[id];
		}

		inline bool remove(SIZE_TYPE id)
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			_free_list[_free_list_head] = id;
			_free_list_head++;
			std::destroy_at(&_data[id]);
			_actives[id] = 0;
			_size--;
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

		inline bool empty() const
		{
			return _size == 0;
		}

		inline bool contains_holes() const
		{
			return _size != _head;
		}

		inline void resize_zero()
		{
			for (SIZE_TYPE i = 0; i < _head; i++)
			{
				if (_actives[i] == 0)
					continue;

				std::destroy_at(&_data[i]);
				_actives[i] = 0;
			}

			_size			= 0;
			_head			= 0;
			_free_list_head = 0;
		}

		inline void clear()
		{
			if (_data)
			{
				for (SIZE_TYPE i = 0; i < _head; i++)
				{
					if (_actives[i] != 0)
						std::destroy_at(&_data[i]);
				}

				::operator delete(_data);
				delete[] _free_list;
				delete[] _actives;
			}

			_head = _capacity = 0;
			_free_list_head	  = 0;
			_size			  = 0;
			_data			  = nullptr;
			_free_list		  = nullptr;
			_actives		  = nullptr;
		}

		template <typename TYPE> struct iterator_t
		{
			using reference = TYPE&;
			using pointer	= TYPE*;

			iterator_t(pointer ptr, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _ptr(ptr), _actives(actives), _current(begin), _end(end)
			{
				while (_current != _end && _actives[_current] == 0)
					++_current;
			}

			reference operator*() const
			{
				return *(_ptr + _current);
			}

			pointer operator->()
			{
				return _ptr + _current;
			}

			iterator_t& operator++()
			{
				do
				{
					_current++;
				} while (_current != _end && _actives[_current] == 0);

				return *this;
			}

			iterator_t operator++(int)
			{
				iterator_t tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const iterator_t& a, const iterator_t& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const iterator_t& a, const iterator_t& b)
			{
				return a._current != b._current;
			}

			pointer	  _ptr	   = nullptr;
			const u8* _actives = nullptr;
			SIZE_TYPE _current = 0;
			SIZE_TYPE _end	   = 0;
		};

		iterator_t<T> begin()
		{
			return iterator_t<T>(_data, _actives, 0, _head);
		}

		iterator_t<T> end()
		{
			return iterator_t<T>(_data, _actives, _head, _head);
		}

		iterator_t<const T> begin() const
		{
			return iterator_t<const T>(_data, _actives, 0, _head);
		}

		iterator_t<const T> end() const
		{
			return iterator_t<const T>(_data, _actives, _head, _head);
		}

	private:
		inline void check_grow(SIZE_TYPE desired_cap = 0)
		{
			if (_head < _capacity && desired_cap < _capacity)
				return;

			const SIZE_TYPE new_cap		  = desired_cap != 0 ? desired_cap : (_capacity == 0 ? 4 : (_capacity * 2));
			T*				new_data	  = static_cast<T*>(::operator new(sizeof(T) * new_cap));
			SIZE_TYPE*		new_free_list = new SIZE_TYPE[new_cap];
			u8*				new_actives	  = new u8[new_cap];

			if (_data)
			{
				if (_head != 0)
				{
					SFG_MEMCPY(new_actives, _actives, sizeof(u8) * _head);
					for (SIZE_TYPE i = 0; i < _head; i++)
					{
						if (_actives[i] == 0)
							continue;

						std::construct_at(&new_data[i], std::move_if_noexcept(_data[i]));
						std::destroy_at(&_data[i]);
					}
				}

				if (_free_list_head != 0)
					SFG_MEMCPY(new_free_list, _free_list, sizeof(SIZE_TYPE) * _free_list_head);

				::operator delete(_data);
				delete[] _free_list;
				delete[] _actives;
				_data	   = nullptr;
				_free_list = nullptr;
				_actives   = nullptr;
			}

			_data	   = new_data;
			_free_list = new_free_list;
			_actives   = new_actives;
			_capacity  = new_cap;

			for (SIZE_TYPE i = _head; i < new_cap; i++)
				_actives[i] = 0;
		}

		inline void move_from(dynamic_pool_allocator_t& other)
		{
			_data				  = other._data;
			_free_list			  = other._free_list;
			_actives			  = other._actives;
			_free_list_head		  = other._free_list_head;
			_head				  = other._head;
			_capacity			  = other._capacity;
			_size				  = other._size;
			other._data			  = nullptr;
			other._free_list	  = nullptr;
			other._actives		  = nullptr;
			other._free_list_head = 0;
			other._head			  = 0;
			other._size			  = 0;
			other._capacity		  = 0;
		}

	private:
		T*		   _data		   = nullptr;
		SIZE_TYPE* _free_list	   = nullptr;
		u8*		   _actives		   = nullptr;
		SIZE_TYPE  _free_list_head = 0;
		SIZE_TYPE  _head		   = 0;
		SIZE_TYPE  _size		   = 0;
		SIZE_TYPE  _capacity	   = 0;
	};

	template <typename T, typename SIZE_TYPE = u32> using dynamic_pool_t = dynamic_pool_allocator_t<T, SIZE_TYPE>;

}
