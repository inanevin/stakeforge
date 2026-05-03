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

#include "pool_handle.hpp"
#include "memory.hpp"
#include <sfg/io/assert.hpp>
#include <new>

namespace sfg
{
	template <typename T, typename SIZE_TYPE, int N, typename TAG = T> struct inplace_gen_pool_t
	{
		using HANDLE   = pool_handle_t<SIZE_TYPE, TAG>;
		using handle_t = HANDLE;

	private:
		struct storage_t
		{
			alignas(T) unsigned char bytes[sizeof(T)];
		};

	public:
		~inplace_gen_pool_t()
		{
			reset();
		}

		inplace_gen_pool_t()
		{
			for (u32 i = 0; i < N; i++)
			{
				_generations[i] = 1;
				_free_list[i]	= 0;
				_actives[i]		= 0;
			}
			_free_count = 0;
		}

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		inline HANDLE add()
		{
			return emplace();
		}

		template <typename... Args> inline HANDLE emplace(Args&&... args)
		{
			if (_free_count > 0)
			{
				const SIZE_TYPE index = _free_list[_free_count - 1];
				_free_count--;
				std::construct_at(ptr(index), std::forward<Args>(args)...);
				_actives[index] = 1;
				_size++;
				return {
					.generation = _generations[index],
					.index		= index,
				};
			}

			SFG_ASSERT(_head < N);

			const SIZE_TYPE index = _head;
			std::construct_at(ptr(index), std::forward<Args>(args)...);
			_actives[index] = 1;
			_head++;
			_size++;
			return {
				.generation = _generations[index],
				.index		= index,
			};
		}

		inline bool is_full() const
		{
			return _free_count == 0 && _head >= N;
		}

		void remove(HANDLE handle)
		{
			SFG_ASSERT(is_valid(handle));
			_free_list[_free_count] = handle.index;
			_free_count++;
			increment_generation(_generations[handle.index]);
			std::destroy_at(ptr(handle.index));
			_actives[handle.index] = 0;
			_size--;
		}

		inline bool is_valid(HANDLE handle) const
		{
			return handle.generation != 0 && handle.index < _head && _actives[handle.index] != 0 && _generations[handle.index] == handle.generation;
		}

		void reset()
		{
			for (SIZE_TYPE i = 0; i < _head; i++)
			{
				if (_actives[i] != 0)
				{
					std::destroy_at(ptr(i));
					increment_generation(_generations[i]);
				}

				_free_list[i] = 0;
				_actives[i]	  = 0;
			}

			_head		= 0;
			_free_count = 0;
			_size		= 0;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		T& get(HANDLE handle)
		{
			SFG_ASSERT(is_valid(handle));
			return *ptr(handle.index);
		}

		const T& get(HANDLE handle) const
		{
			SFG_ASSERT(is_valid(handle));
			return *ptr(handle.index);
		}

		inline SIZE_TYPE get_generation(SIZE_TYPE index) const
		{
			SFG_ASSERT(index < N);
			return _generations[index];
		};

		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		template <typename TYPE> struct iterator_t
		{
			using reference		= TYPE&;
			using pointer		= TYPE*;
			using storage_ptr_t = std::conditional_t<std::is_const_v<TYPE>, const storage_t*, storage_t*>;

			iterator_t(storage_ptr_t storage, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _storage(storage), _actives(actives), _current(begin), _end(end)
			{
				while (_current != end && actives[_current] == 0)
					++_current;
			}

			reference operator*() const
			{
				return *std::launder(reinterpret_cast<pointer>(&_storage[_current]));
			};
			pointer operator->()
			{
				return std::launder(reinterpret_cast<pointer>(&_storage[_current]));
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

			storage_ptr_t _storage = nullptr;
			const u8*	  _actives = nullptr;
			SIZE_TYPE	  _current = 0;
			SIZE_TYPE	  _end	   = 0;
		};

		iterator_t<const T> begin() const
		{
			return iterator_t<const T>(_items, _actives, 0, _head);
		}

		iterator_t<const T> end() const
		{
			return iterator_t<const T>(_items, _actives, _head, _head);
		}

		iterator_t<T> begin()
		{
			return iterator_t<T>(_items, _actives, 0, _head);
		}

		iterator_t<T> end()
		{
			return iterator_t<T>(_items, _actives, _head, _head);
		}

		// -----------------------------------------------------------------------------
		// handle iterators
		// -----------------------------------------------------------------------------

		struct handle_iterator_t
		{
			handle_iterator_t(const SIZE_TYPE* gens, const u8* actives, SIZE_TYPE begin, SIZE_TYPE end) : _gens(gens), _actives(actives), _current(begin), _end(end)
			{
				while (_current != end && actives[_current] == 0)
					++_current;
			}

			HANDLE operator*() const
			{
				return {
					.generation = _gens[_current],
					.index		= _current,
				};
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

			const u8*		 _actives = nullptr;
			const SIZE_TYPE* _gens	  = nullptr;
			SIZE_TYPE		 _current = 0;
			SIZE_TYPE		 _end	  = 0;
		};

		handle_iterator_t handles_begin() const
		{
			return handle_iterator_t(_generations, _actives, 0, _head);
		}

		handle_iterator_t handles_end() const
		{
			return handle_iterator_t(_generations, _actives, _head, _head);
		}

		handle_iterator_t begin_handle() const
		{
			return handles_begin();
		}

		handle_iterator_t end_handle() const
		{
			return handles_end();
		}

	private:
		T* ptr(SIZE_TYPE index)
		{
			return std::launder(reinterpret_cast<T*>(&_items[index]));
		}

		const T* ptr(SIZE_TYPE index) const
		{
			return std::launder(reinterpret_cast<const T*>(&_items[index]));
		}

		void increment_generation(SIZE_TYPE& generation)
		{
			generation++;
			if (generation == 0)
				generation++;
		}

	private:
		storage_t _items[N];
		SIZE_TYPE _free_count = 0;
		SIZE_TYPE _free_list[N];
		SIZE_TYPE _generations[N];
		u8		  _actives[N];
		SIZE_TYPE _head = 0;
		SIZE_TYPE _size = 0;
	};

}
