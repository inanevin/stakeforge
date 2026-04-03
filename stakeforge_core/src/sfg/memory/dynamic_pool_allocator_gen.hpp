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
#include "pool_handle.hpp"

namespace sfg
{
	template <typename T, typename SIZE_TYPE = u32> struct dynamic_pool_allocator_gen_t
	{
		static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>);
		static_assert(std::is_integral_v<SIZE_TYPE> && std::is_unsigned_v<SIZE_TYPE>);

		struct slot_t
		{
			SIZE_TYPE dense_index = 0;
			SIZE_TYPE generation  = 1;
			u8		  active	   = 0;
		};

		template <typename TYPE> struct iterator_t
		{
			using reference = TYPE&;
			using pointer	= TYPE*;

			iterator_t(pointer ptr, SIZE_TYPE begin, SIZE_TYPE end) : _ptr(ptr), _current(begin), _end(end)
			{
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
				_current++;
				return *this;
			}

			iterator_t& operator++(int)
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
			SIZE_TYPE _current = 0;
			SIZE_TYPE _end	   = 0;
		};

		using handle_t		 = pool_handle_t<SIZE_TYPE>;
		using iterator		 = iterator_t<T>;
		using const_iterator = iterator_t<const T>;
		using handle_iterator = iterator_t<const handle_t>;

		~dynamic_pool_allocator_gen_t()
		{
			uninit();
		}

		dynamic_pool_allocator_gen_t() = default;

		dynamic_pool_allocator_gen_t(const dynamic_pool_allocator_gen_t&) = delete;
		dynamic_pool_allocator_gen_t& operator=(const dynamic_pool_allocator_gen_t&) = delete;

		dynamic_pool_allocator_gen_t(dynamic_pool_allocator_gen_t&& other) noexcept
		{
			move_from(other);
		}

		dynamic_pool_allocator_gen_t& operator=(dynamic_pool_allocator_gen_t&& other) noexcept
		{
			if (this == &other)
				return *this;

			uninit();
			move_from(other);
			return *this;
		}

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		handle_t add()
		{
			const SIZE_TYPE dense_index = _size;
			const SIZE_TYPE slot_index  = allocate_slot(dense_index);

			ensure_item_capacity(dense_index + 1);

			const handle_t handle {
				.generation = _slots[slot_index].generation,
				.index		= slot_index,
			};

			_items[dense_index]		= T();
			_handles[dense_index]	= handle;
			_size++;
			return handle;
		}

		handle_t add(const T& item)
		{
			const handle_t handle = add();
			_items[_size - 1]	   = item;
			return handle;
		}

		void remove(handle_t handle)
		{
			SFG_ASSERT(is_valid(handle));

			slot_t& slot				 = _slots[handle.index];
			const SIZE_TYPE dense_index = slot.dense_index;
			const SIZE_TYPE last_index	 = _size - 1;

			if (dense_index != last_index)
			{
				_items[dense_index] = _items[last_index];

				const handle_t moved_handle = _handles[last_index];
				_handles[dense_index]	   = moved_handle;
				_slots[moved_handle.index].dense_index = dense_index;
			}

			_size--;
			slot.active	   = 0;
			slot.dense_index = 0;
			increment_generation(slot.generation);
			_free_list[_free_count] = handle.index;
			_free_count++;
		}

		void reset()
		{
			_size	   = 0;
			_free_count = _slot_count;

			for (SIZE_TYPE i = 0; i < _slot_count; i++)
			{
				_slots[i].dense_index				   = 0;
				_slots[i].active					   = 0;
				increment_generation(_slots[i].generation);
				_free_list[_slot_count - i - 1] = i;
			}
		}

		void reserve(SIZE_TYPE size)
		{
			ensure_item_capacity(size);
			ensure_slot_capacity(size);
		}

		void uninit()
		{
			if (_items != nullptr)
				SFG_FREE(_items);
			if (_handles != nullptr)
				SFG_FREE(_handles);
			if (_slots != nullptr)
				SFG_FREE(_slots);
			if (_free_list != nullptr)
				SFG_FREE(_free_list);

			_items		   = nullptr;
			_handles	   = nullptr;
			_slots		   = nullptr;
			_free_list	   = nullptr;
			_size		   = 0;
			_capacity	   = 0;
			_slot_count	   = 0;
			_slot_capacity  = 0;
			_free_count	   = 0;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool is_valid(handle_t handle) const
		{
			if (handle.is_null())
				return false;

			if (handle.index >= _slot_count)
				return false;

			const slot_t& slot = _slots[handle.index];
			return slot.active != 0 && slot.generation == handle.generation;
		}

		T& get(handle_t handle)
		{
			SFG_ASSERT(is_valid(handle));
			return _items[_slots[handle.index].dense_index];
		}

		const T& get(handle_t handle) const
		{
			SFG_ASSERT(is_valid(handle));
			return _items[_slots[handle.index].dense_index];
		}

		handle_t get_handle(SIZE_TYPE dense_index) const
		{
			SFG_ASSERT(dense_index < _size);
			return _handles[dense_index];
		}

		SIZE_TYPE size() const
		{
			return _size;
		}

		SIZE_TYPE capacity() const
		{
			return _capacity;
		}

		SIZE_TYPE slot_count() const
		{
			return _slot_count;
		}

		bool is_empty() const
		{
			return _size == 0;
		}

		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		const_iterator begin() const
		{
			return const_iterator(_items, 0, _size);
		}

		const_iterator end() const
		{
			return const_iterator(_items, _size, _size);
		}

		iterator begin()
		{
			return iterator(_items, 0, _size);
		}

		iterator end()
		{
			return iterator(_items, _size, _size);
		}

		handle_iterator handles_begin() const
		{
			return handle_iterator(_handles, 0, _size);
		}

		handle_iterator handles_end() const
		{
			return handle_iterator(_handles, _size, _size);
		}

	private:
		void move_from(dynamic_pool_allocator_gen_t& other)
		{
			_items		   = other._items;
			_handles	   = other._handles;
			_slots		   = other._slots;
			_free_list	   = other._free_list;
			_size		   = other._size;
			_capacity	   = other._capacity;
			_slot_count	   = other._slot_count;
			_slot_capacity  = other._slot_capacity;
			_free_count	   = other._free_count;

			other._items		   = nullptr;
			other._handles	   = nullptr;
			other._slots		   = nullptr;
			other._free_list	   = nullptr;
			other._size		   = 0;
			other._capacity	   = 0;
			other._slot_count	   = 0;
			other._slot_capacity  = 0;
			other._free_count	   = 0;
		}

		SIZE_TYPE allocate_slot(SIZE_TYPE dense_index)
		{
			if (_free_count != 0)
			{
				const SIZE_TYPE slot_index = _free_list[_free_count - 1];
				_free_count--;

				_slots[slot_index].dense_index = dense_index;
				_slots[slot_index].active	  = 1;
				return slot_index;
			}

			const SIZE_TYPE slot_index = _slot_count;
			ensure_slot_capacity(slot_index + 1);

			_slots[slot_index].dense_index = dense_index;
			_slots[slot_index].generation  = 1;
			_slots[slot_index].active	  = 1;
			_slot_count++;
			return slot_index;
		}

		void ensure_item_capacity(SIZE_TYPE size)
		{
			if (size <= _capacity)
				return;

			SIZE_TYPE new_capacity = _capacity == 0 ? 8 : _capacity * 2;
			while (new_capacity < size)
				new_capacity *= 2;

			T* new_items			   = static_cast<T*>(SFG_MALLOC(sizeof(T) * new_capacity));
			handle_t* new_handles	   = static_cast<handle_t*>(SFG_MALLOC(sizeof(handle_t) * new_capacity));

			SFG_ASSERT(new_items != nullptr);
			SFG_ASSERT(new_handles != nullptr);

			if (_size != 0)
			{
				SFG_MEMCPY(new_items, _items, sizeof(T) * _size);
				SFG_MEMCPY(new_handles, _handles, sizeof(handle_t) * _size);
			}

			if (_items != nullptr)
				SFG_FREE(_items);
			if (_handles != nullptr)
				SFG_FREE(_handles);

			_items	 = new_items;
			_handles = new_handles;
			_capacity = new_capacity;
		}

		void ensure_slot_capacity(SIZE_TYPE size)
		{
			if (size <= _slot_capacity)
				return;

			SIZE_TYPE new_capacity = _slot_capacity == 0 ? 8 : _slot_capacity * 2;
			while (new_capacity < size)
				new_capacity *= 2;

			slot_t* new_slots			 = static_cast<slot_t*>(SFG_MALLOC(sizeof(slot_t) * new_capacity));
			SIZE_TYPE* new_free_list = static_cast<SIZE_TYPE*>(SFG_MALLOC(sizeof(SIZE_TYPE) * new_capacity));

			SFG_ASSERT(new_slots != nullptr);
			SFG_ASSERT(new_free_list != nullptr);

			if (_slot_count != 0)
			{
				SFG_MEMCPY(new_slots, _slots, sizeof(slot_t) * _slot_count);
				SFG_MEMCPY(new_free_list, _free_list, sizeof(SIZE_TYPE) * _free_count);
			}

			if (_slots != nullptr)
				SFG_FREE(_slots);
			if (_free_list != nullptr)
				SFG_FREE(_free_list);

			_slots		  = new_slots;
			_free_list	  = new_free_list;
			_slot_capacity = new_capacity;
		}

		void increment_generation(SIZE_TYPE& generation)
		{
			generation++;
			if (generation == 0)
				generation++;
		}

	private:
		T*		  _items		   = nullptr;
		handle_t* _handles	   = nullptr;
		slot_t*	  _slots		   = nullptr;
		SIZE_TYPE* _free_list	   = nullptr;
		SIZE_TYPE _size		   = 0;
		SIZE_TYPE _capacity	   = 0;
		SIZE_TYPE _slot_count	   = 0;
		SIZE_TYPE _slot_capacity  = 0;
		SIZE_TYPE _free_count	   = 0;
	};

	template <typename T, typename SIZE_TYPE = u32> using dynamic_pool_allocator_gen = dynamic_pool_allocator_gen_t<T, SIZE_TYPE>;

}
