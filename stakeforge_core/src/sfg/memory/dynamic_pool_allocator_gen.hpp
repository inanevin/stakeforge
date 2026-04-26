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
#include "pool_handle.hpp"
#include "io/assert.hpp"
#include "memory.hpp"

namespace sfg
{

	template <typename T, typename SIZE_TYPE = u32, typename TAG = T> class dynamic_pool_allocator_gen_t
	{

	private:
		static_assert(std::is_integral_v<SIZE_TYPE>);
		static_assert(std::is_unsigned_v<SIZE_TYPE>);
		static_assert(std::is_nothrow_move_constructible_v<T> || std::is_copy_constructible_v<T>, "dynamic_pool_allocator_gen_t requires T to be nothrow move constructible or copy constructible");

	public:
		using HANDLE   = pool_handle_t<SIZE_TYPE, TAG>;

		dynamic_pool_allocator_gen_t()													   = default;
		dynamic_pool_allocator_gen_t(const dynamic_pool_allocator_gen_t& other)			   = delete;
		dynamic_pool_allocator_gen_t& operator=(const dynamic_pool_allocator_gen_t& other) = delete;

		dynamic_pool_allocator_gen_t(dynamic_pool_allocator_gen_t&& other) noexcept
		{
			move_from(other);
		}

		~dynamic_pool_allocator_gen_t()
		{
			clear();
		}

		dynamic_pool_allocator_gen_t& operator=(dynamic_pool_allocator_gen_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			clear();
			move_from(other);
			return *this;
		}

		inline HANDLE add()
		{
			return emplace();
		}

		template <typename... Args> inline HANDLE emplace(Args&&... args)
		{
			if (_free_list_head != 0)
			{
				const SIZE_TYPE idx = _free_list[_free_list_head - 1];
				--_free_list_head;
				std::construct_at(&(_data[idx]), std::forward<Args>(args)...);
				_actives[idx] = 1;
				_size++;

				return {.generation = _generations[idx], .index = idx};
			}

			check_grow();

			const SIZE_TYPE idx = _head;
			_head++;
			_size++;
			std::construct_at(&(_data[idx]), std::forward<Args>(args)...);
			_actives[idx] = 1;
			return {.generation = _generations[idx], .index = idx};
		}

		inline const T& get(SIZE_TYPE index) const
		{
			SFG_ASSERT(index < _head && _actives[index] != 0);
			return _data[index];
		}

		inline const T& get(HANDLE handle) const
		{
			SFG_ASSERT(is_valid(handle));
			return _data[handle.index];
		}

		inline T& get(SIZE_TYPE index)
		{
			SFG_ASSERT(index < _head && _actives[index] != 0);
			return _data[index];
		}

		inline T& get(HANDLE handle)
		{
			SFG_ASSERT(is_valid(handle));
			return _data[handle.index];
		}

		inline bool is_valid(HANDLE handle) const
		{
			return handle.generation != 0 && handle.index < _head && _actives[handle.index] != 0 && handle.generation == _generations[handle.index];
		}

		inline bool is_active(SIZE_TYPE index) const
		{
			SFG_ASSERT(index < _head);
			return _actives[index] != 0;
		}

		inline HANDLE get_handle(SIZE_TYPE index) const
		{
			SFG_ASSERT(index < _head && _actives[index] != 0);
			return {.generation = _generations[index], .index = index};
		}

		inline bool remove(HANDLE handle)
		{
			SFG_ASSERT(is_valid(handle));
			_free_list[_free_list_head] = handle.index;
			_free_list_head++;
			if constexpr (!std::is_trivially_destructible_v<T>)
				std::destroy_at(&_data[handle.index]);
			_actives[handle.index] = 0;
			increment_generation(_generations[handle.index]);
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

		inline SIZE_TYPE head() const
		{
			return _head;
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

				if constexpr (!std::is_trivially_destructible_v<T>)
					std::destroy_at(&_data[i]);
				_actives[i] = 0;
				increment_generation(_generations[i]);
			}

			_head			= 0;
			_size			= 0;
			_free_list_head = 0;
		}

		inline void clear()
		{
			if (_data)
			{
				if constexpr (!std::is_trivially_destructible_v<T>)
				{
					for (SIZE_TYPE i = 0; i < _head; i++)
					{
						if (_actives[i] != 0)
							std::destroy_at(&_data[i]);
					}
				}

				deallocate_data(_data);
				delete[] _free_list;
				delete[] _actives;
				delete[] _generations;
			}

			_head = _capacity = 0;
			_size			  = 0;
			_free_list_head	  = 0;
			_data			  = nullptr;
			_free_list		  = nullptr;
			_actives		  = nullptr;
			_generations	  = nullptr;
		}

		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		struct iterator_t
		{
			using reference = T&;
			using pointer	= T*;

			iterator_t(pointer ptr, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _ptr(ptr), _actives(actives), _current(begin), _end(end)
			{
				while (_current != end && _actives[_current] == 0)
					++_current;
			}

			reference operator*() const
			{
				return *(_ptr + _current);
			}

			pointer operator->()
			{
				return (_ptr + _current);
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

		iterator_t begin()
		{
			return iterator_t(_data, _actives, 0, _head);
		}

		iterator_t end()
		{
			return iterator_t(_data, _actives, _head, _head);
		}

		struct const_iterator_t
		{
			using reference = const T&;
			using pointer	= const T*;

			const_iterator_t(pointer ptr, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _ptr(ptr), _actives(actives), _current(begin), _end(end)
			{
				while (_current != end && _actives[_current] == 0)
					++_current;
			}

			reference operator*() const
			{
				return *(_ptr + _current);
			}

			pointer operator->()
			{
				return (_ptr + _current);
			}

			const_iterator_t& operator++()
			{
				do
				{
					_current++;
				} while (_current != _end && _actives[_current] == 0);

				return *this;
			}

			const_iterator_t operator++(int)
			{
				const_iterator_t tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const const_iterator_t& a, const const_iterator_t& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const const_iterator_t& a, const const_iterator_t& b)
			{
				return a._current != b._current;
			}

			pointer	  _ptr	   = nullptr;
			const u8* _actives = nullptr;
			SIZE_TYPE _current = 0;
			SIZE_TYPE _end	   = 0;
		};

		const_iterator_t begin() const
		{
			return const_iterator_t(_data, _actives, 0, _head);
		}

		const_iterator_t end() const
		{
			return const_iterator_t(_data, _actives, _head, _head);
		}

		struct handle_iterator_t
		{
			handle_iterator_t(const SIZE_TYPE* generations, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _generations(generations), _actives(actives), _current(begin), _end(end)
			{
				while (_current != _end && _actives[_current] == 0)
					++_current;
			}

			HANDLE operator*() const
			{
				return {.generation = _generations[_current], .index = _current};
			}

			handle_iterator_t& operator++()
			{
				do
				{
					_current++;
				} while (_current != _end && _actives[_current] == 0);

				return *this;
			}

			handle_iterator_t operator++(int)
			{
				handle_iterator_t tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const handle_iterator_t& a, const handle_iterator_t& b)
			{
				return a._current == b._current;
			}

			friend bool operator!=(const handle_iterator_t& a, const handle_iterator_t& b)
			{
				return a._current != b._current;
			}

			const SIZE_TYPE* _generations = nullptr;
			const u8*		 _actives	  = nullptr;
			SIZE_TYPE		 _current	  = 0;
			SIZE_TYPE		 _end		  = 0;
		};

		handle_iterator_t begin_handle() const
		{
			return handle_iterator_t(_generations, _actives, 0, _head);
		}

		handle_iterator_t end_handle() const
		{
			return handle_iterator_t(_generations, _actives, _head, _head);
		}

	private:
		inline void check_grow(SIZE_TYPE desired_cap = 0)
		{
			if ((desired_cap == 0 && _head < _capacity) || (desired_cap != 0 && desired_cap <= _capacity))
				return;

			const SIZE_TYPE new_cap		  = desired_cap != 0 ? desired_cap : (_capacity == 0 ? 4 : (_capacity * 2));
			T*				new_data	  = allocate_data(new_cap);
			SIZE_TYPE*		new_free_list = new SIZE_TYPE[new_cap];
			SIZE_TYPE*		new_gens	  = new SIZE_TYPE[new_cap];
			u8*				new_actives	  = new u8[new_cap];
			if (_data)
			{
				if (_head != 0)
				{
					SFG_MEMCPY(new_gens, _generations, sizeof(SIZE_TYPE) * _head);
					SFG_MEMCPY(new_actives, _actives, sizeof(u8) * _head);
					if constexpr (std::is_trivially_copyable_v<T>)
					{
						SFG_MEMCPY(new_data, _data, sizeof(T) * _head);
					}
					else
					{
						for (SIZE_TYPE i = 0; i < _head; i++)
						{
							if (_actives[i] == 0)
								continue;

							std::construct_at(&new_data[i], std::move_if_noexcept(_data[i]));
							std::destroy_at(&_data[i]);
						}
					}
				}

				if (_free_list_head != 0)
					SFG_MEMCPY(new_free_list, _free_list, sizeof(SIZE_TYPE) * _free_list_head);

				deallocate_data(_data);
				delete[] _free_list;
				delete[] _actives;
				delete[] _generations;
				_data		 = nullptr;
				_free_list	 = nullptr;
				_actives	 = nullptr;
				_generations = nullptr;
			}

			_data		 = new_data;
			_free_list	 = new_free_list;
			_capacity	 = new_cap;
			_generations = new_gens;
			_actives	 = new_actives;

			for (SIZE_TYPE i = _head; i < new_cap; i++)
			{
				_generations[i] = 1;
				_actives[i]		= 0;
			}
		}

		inline void move_from(dynamic_pool_allocator_gen_t& other)
		{
			_data				  = other._data;
			_free_list			  = other._free_list;
			_generations		  = other._generations;
			_actives			  = other._actives;
			_free_list_head		  = other._free_list_head;
			_head				  = other._head;
			_size				  = other._size;
			_capacity			  = other._capacity;
			other._data			  = nullptr;
			other._free_list	  = nullptr;
			other._generations	  = nullptr;
			other._actives		  = nullptr;
			other._free_list_head = 0;
			other._head			  = 0;
			other._size			  = 0;
			other._capacity		  = 0;
		}

		inline void increment_generation(SIZE_TYPE& generation)
		{
			generation++;
			if (generation == 0)
				generation++;
		}

		static T* allocate_data(SIZE_TYPE capacity)
		{
			return static_cast<T*>(::operator new(sizeof(T) * capacity, std::align_val_t(alignof(T))));
		}

		static void deallocate_data(T* data)
		{
			::operator delete(data, std::align_val_t(alignof(T)));
		}

	private:
		T*		   _data		   = nullptr;
		SIZE_TYPE* _free_list	   = nullptr;
		SIZE_TYPE* _generations	   = nullptr;
		u8*		   _actives		   = nullptr;
		SIZE_TYPE  _free_list_head = 0;
		SIZE_TYPE  _head		   = 0;
		SIZE_TYPE  _size		   = 0;
		SIZE_TYPE  _capacity	   = 0;
	};

	template <typename T, typename SIZE_TYPE = u32, typename TAG = T> using dynamic_pool_gen_t = dynamic_pool_allocator_gen_t<T, SIZE_TYPE, TAG>;

}
