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

#include "io/assert.hpp"
#include <initializer_list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace sfg
{
	template <typename T, int N> class static_vector_t
	{
		static_assert(N >= 0);

	public:
		using value_type	  = T;
		using size_type		  = std::size_t;
		using reference		  = value_type&;
		using const_reference = const value_type&;
		using iterator_t	  = T*;
		using const_iterator  = const T*;

		static constexpr size_type capacity = N;

		static_vector_t() : _head(0)
		{
		}

		static_vector_t(value_type v) : _head(0)
		{
			for (int i = 0; i < capacity; i++)
				emplace_back(v);
		}

		static_vector_t(std::initializer_list<T> ilist)
		{
			_head = 0;
			SFG_ASSERT(ilist.size() <= N && "initializer list too big");
			for (auto&& e : ilist)
				emplace_back(e);
		}

		static_vector_t(const static_vector_t& other) : _head(0)
		{
			for (const T& value : other)
				emplace_back(value);
		}

		static_vector_t& operator=(const static_vector_t& other)
		{
			if (this == &other)
				return *this;

			clear();
			for (const T& value : other)
				emplace_back(value);
			return *this;
		}

		static_vector_t(static_vector_t&& other) noexcept(std::is_nothrow_move_constructible_v<T>) : _head(0)
		{
			for (T& value : other)
				emplace_back(std::move(value));
			other.clear();
		}

		static_vector_t& operator=(static_vector_t&& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
		{
			if (this == &other)
				return *this;

			clear();
			for (T& value : other)
				emplace_back(std::move(value));
			other.clear();
			return *this;
		}

		~static_vector_t()
		{
			clear();
		}

		reference operator[](size_type index)
		{
			return at(index);
		}
		const_reference operator[](size_type index) const
		{
			return at(index);
		}

		reference at(size_type index)
		{
			if (index >= size())
			{
				SFG_ASSERT(false, "");
				return *ptr(0);
			}
			return *ptr(index);
		}

		const_reference at(size_type index) const
		{
			if (index >= size())
			{
				SFG_ASSERT(false, "");
				return *ptr(0);
			}
			return *ptr(index);
		}

		reference front()
		{
			SFG_ASSERT(!empty());
			return *ptr(0);
		}
		const_reference front() const
		{
			SFG_ASSERT(!empty());
			return *ptr(0);
		}

		reference back()
		{
			SFG_ASSERT(!empty());
			return *ptr(_head - 1);
		}
		const_reference back() const
		{
			SFG_ASSERT(!empty());
			return *ptr(_head - 1);
		}

		iterator_t begin()
		{
			return data();
		}
		const_iterator begin() const
		{
			return data();
		}
		const_iterator cbegin() const
		{
			return data();
		}

		iterator_t end()
		{
			return data() + _head;
		}
		const_iterator end() const
		{
			return data() + _head;
		}
		const_iterator cend() const
		{
			return data() + _head;
		}

		size_type size() const
		{
			return _head;
		}
		bool empty() const
		{
			return _head == 0;
		}
		bool full() const
		{
			return _head == N;
		}

		void push_back(const T& value)
		{
			emplace_back(value);
		}

		void push_back(T&& value)
		{
			emplace_back(std::move(value));
		}

		template <typename... Args> reference emplace_back(Args&&... args)
		{
			SFG_ASSERT(!full());
			T* inserted = ptr(_head);
			std::construct_at(inserted, std::forward<Args>(args)...);
			_head++;
			return *inserted;
		}

		void pop_back()
		{
			if (!empty())
			{
				--_head;
				std::destroy_at(ptr(_head));
			}
		}

		void clear()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (size_type i = 0; i < _head; i++)
					std::destroy_at(ptr(i));
			}

			_head = 0;
		}

		void resize(size_t sz)
		{
			SFG_ASSERT(sz <= capacity);
			while (_head > sz)
				pop_back();

			while (_head < sz)
				emplace_back();

			_head = sz;
		}

		// Linear search for value. Returns end() if not found.
		iterator_t find(const T& value)
		{
			for (size_type i = 0; i < _head; ++i)
				if (*ptr(i) == value)
					return data() + i;
			return end();
		}

		const_iterator find(const T& value) const
		{
			for (size_type i = 0; i < _head; ++i)
				if (*ptr(i) == value)
					return data() + i;
			return end();
		}

		// Linear search by predicate. Returns end() if not found.
		template <typename Pred> iterator_t find_if(Pred pred)
		{
			for (size_type i = 0; i < _head; ++i)
				if (pred(*ptr(i)))
					return data() + i;
			return end();
		}

		template <typename Pred> const_iterator find_if(Pred pred) const
		{
			for (size_type i = 0; i < _head; ++i)
				if (pred(*ptr(i)))
					return data() + i;
			return end();
		}

		void remove(const T& value)
		{
			auto it = find(value);
			SFG_ASSERT(it != end());
			const size_t idx = static_cast<size_t>(it - begin());
			remove_index(idx);
		}

		void remove_swap(const T& value)
		{
			auto it = find(value);
			SFG_ASSERT(it != end());
			const size_t idx = static_cast<size_t>(it - begin());
			remove_index_swap(idx);
		}

		template <typename Pred> void remove_if(Pred pred)
		{
			auto it = find_if(pred);
			SFG_ASSERT(it != end());
			const size_t idx = static_cast<size_t>(it - begin());
			remove_index(idx);
		}

		template <typename Pred> void remove_if_swap(Pred pred)
		{
			auto it = find_if(pred);
			SFG_ASSERT(it != end());
			const size_t idx = static_cast<size_t>(it - begin());
			remove_index_swap(idx);
		}

		void remove_index(size_t idx)
		{
			SFG_ASSERT(idx < _head);
			for (size_t i = idx; i < _head - 1; i++)
				*ptr(i) = std::move(*ptr(i + 1));

			pop_back();
		}

		void remove_index_swap(size_t idx)
		{
			SFG_ASSERT(idx < _head);

			if (idx < _head - 1)
				*ptr(idx) = std::move(*ptr(_head - 1));

			pop_back();
		}

		T* data()
		{
			return ptr(0);
		}
		const T* data() const
		{
			return ptr(0);
		}

	private:
		struct storage_t
		{
			alignas(T) unsigned char bytes[sizeof(T)];
		};

		T* ptr(size_type index)
		{
			return std::launder(reinterpret_cast<T*>(&_data[index]));
		}

		const T* ptr(size_type index) const
		{
			return std::launder(reinterpret_cast<const T*>(&_data[index]));
		}

	private:
		storage_t _data[N] = {};
		size_t	  _head	   = 0;
	};
}
