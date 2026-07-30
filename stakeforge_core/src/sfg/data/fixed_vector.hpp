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

#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace sfg
{
	template <typename T> class fixed_vector_t
	{
	public:
		using value_type	  = T;
		using size_type		  = std::size_t;
		using reference		  = value_type&;
		using const_reference = const value_type&;
		using iterator_t	  = T*;
		using iterator		  = iterator_t;
		using const_iterator  = const T*;

		fixed_vector_t() = default;
		~fixed_vector_t()
		{
			uninit();
		}
		fixed_vector_t(const fixed_vector_t&)			 = delete;
		fixed_vector_t& operator=(const fixed_vector_t&) = delete;

		inline void init(size_type capacity)
		{
			SFG_ASSERT(_data == nullptr);
			_capacity = capacity;
			_head	  = 0;

			if (_capacity == 0)
				return;

			_data = static_cast<T*>(SFG_ALIGNED_MALLOC(get_allocation_alignment(), get_allocation_size(_capacity)));
			SFG_ASSERT(_data != nullptr);
		}

		inline void uninit()
		{
			clear();
			if (_data != nullptr)
				SFG_ALIGNED_FREE(_data);
			_data	  = nullptr;
			_capacity = 0;
		}

		inline reference operator[](size_type index)
		{
			return at(index);
		}

		inline const_reference operator[](size_type index) const
		{
			return at(index);
		}

		inline reference at(size_type index)
		{
			if (index >= size())
			{
				SFG_ASSERT(false, "");
				return *_data;
			}
			return _data[index];
		}

		inline const_reference at(size_type index) const
		{
			if (index >= size())
			{
				SFG_ASSERT(false, "");
				return *_data;
			}
			return _data[index];
		}

		inline reference front()
		{
			SFG_ASSERT(!empty());
			return _data[0];
		}

		inline const_reference front() const
		{
			SFG_ASSERT(!empty());
			return _data[0];
		}

		inline reference back()
		{
			SFG_ASSERT(!empty());
			return _data[_head - 1];
		}

		inline const_reference back() const
		{
			SFG_ASSERT(!empty());
			return _data[_head - 1];
		}

		inline iterator_t begin()
		{
			return data();
		}

		inline const_iterator begin() const
		{
			return data();
		}

		inline const_iterator cbegin() const
		{
			return data();
		}

		inline iterator_t end()
		{
			return data() == nullptr ? nullptr : data() + _head;
		}

		inline const_iterator end() const
		{
			return data() == nullptr ? nullptr : data() + _head;
		}

		inline const_iterator cend() const
		{
			return data() == nullptr ? nullptr : data() + _head;
		}

		inline size_type size() const
		{
			return _head;
		}

		inline size_type capacity() const
		{
			return _capacity;
		}

		inline bool empty() const
		{
			return _head == 0;
		}

		inline bool full() const
		{
			return _head == _capacity;
		}

		inline void push_back(const T& value)
		{
			emplace_back(value);
		}

		inline void push_back(T&& value)
		{
			emplace_back(std::move(value));
		}

		template <typename... Args> inline reference emplace_back(Args&&... args)
		{
			SFG_ASSERT(!full());
			T* inserted = _data + _head;
			std::construct_at(inserted, std::forward<Args>(args)...);
			_head++;
			return *inserted;
		}

		inline void pop_back()
		{
			SFG_ASSERT(!empty());
			--_head;
			std::destroy_at(_data + _head);
		}

		inline void clear()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (size_type i = 0; i < _head; i++)
					std::destroy_at(_data + i);
			}

			_head = 0;
		}

		inline void resize(size_type sz)
		{
			SFG_ASSERT(sz <= _capacity);
			while (_head > sz)
				pop_back();

			while (_head < sz)
				emplace_back();
		}

		inline iterator_t erase(iterator_t it)
		{
			SFG_ASSERT(it >= begin() && it < end());
			const size_type idx = static_cast<size_type>(it - begin());
			remove_index(idx);
			return begin() + idx;
		}

		inline iterator_t find(const T& value)
		{
			for (size_type i = 0; i < _head; ++i)
			{
				if (_data[i] == value)
					return data() + i;
			}
			return end();
		}

		inline const_iterator find(const T& value) const
		{
			for (size_type i = 0; i < _head; ++i)
			{
				if (_data[i] == value)
					return data() + i;
			}
			return end();
		}

		template <typename Pred> inline iterator_t find_if(Pred pred)
		{
			for (size_type i = 0; i < _head; ++i)
			{
				if (pred(_data[i]))
					return data() + i;
			}
			return end();
		}

		template <typename Pred> inline const_iterator find_if(Pred pred) const
		{
			for (size_type i = 0; i < _head; ++i)
			{
				if (pred(_data[i]))
					return data() + i;
			}
			return end();
		}

		inline void remove(const T& value)
		{
			auto it = find(value);
			SFG_ASSERT(it != end());
			const size_type idx = static_cast<size_type>(it - begin());
			remove_index(idx);
		}

		inline void remove_swap(const T& value)
		{
			auto it = find(value);
			SFG_ASSERT(it != end());
			const size_type idx = static_cast<size_type>(it - begin());
			remove_index_swap(idx);
		}

		template <typename Pred> inline void remove_if(Pred pred)
		{
			auto it = find_if(pred);
			SFG_ASSERT(it != end());
			const size_type idx = static_cast<size_type>(it - begin());
			remove_index(idx);
		}

		template <typename Pred> inline void remove_if_swap(Pred pred)
		{
			auto it = find_if(pred);
			SFG_ASSERT(it != end());
			const size_type idx = static_cast<size_type>(it - begin());
			remove_index_swap(idx);
		}

		inline void remove_index(size_type idx)
		{
			SFG_ASSERT(idx < _head);
			for (size_type i = idx; i < _head - 1; i++)
				_data[i] = std::move(_data[i + 1]);

			pop_back();
		}

		inline void remove_index_swap(size_type idx)
		{
			SFG_ASSERT(idx < _head);

			if (idx < _head - 1)
				_data[idx] = std::move(_data[_head - 1]);

			pop_back();
		}

		inline T* data()
		{
			return _data;
		}

		inline const T* data() const
		{
			return _data;
		}

	private:
		static constexpr size_type get_allocation_alignment()
		{
			return alignof(T) < alignof(void*) ? alignof(void*) : alignof(T);
		}

		static size_type get_allocation_size(size_type capacity)
		{
			const size_type alignment = get_allocation_alignment();
			const size_type bytes	  = sizeof(T) * capacity;
			const size_type remainder = bytes % alignment;
			return remainder == 0 ? bytes : bytes + alignment - remainder;
		}

	private:
		T*		  _data		= nullptr;
		size_type _capacity = 0;
		size_type _head		= 0;
	};
}
