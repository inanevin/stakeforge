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

#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	template <typename T, int N> class inplace_vector_t
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

		inplace_vector_t() : _head(0)
		{
		}

		inplace_vector_t(value_type v) : _head(0)
		{
			for (int i = 0; i < capacity; i++)
				emplace_back(v);
		}

		inplace_vector_t(std::initializer_list<T> ilist)
		{
			_head = 0;
			SFG_ASSERT(ilist.size() <= N && "initializer list too big");
			for (auto&& e : ilist)
				emplace_back(e);
		}

		inplace_vector_t(const inplace_vector_t& other) : _head(0)
		{
			for (const T& value : other)
				emplace_back(value);
		}

		inplace_vector_t& operator=(const inplace_vector_t& other)
		{
			if (this == &other)
				return *this;

			clear();
			for (const T& value : other)
				emplace_back(value);
			return *this;
		}

		inplace_vector_t(inplace_vector_t&& other) noexcept(std::is_nothrow_move_constructible_v<T>) : _head(0)
		{
			for (T& value : other)
				emplace_back(std::move(value));
			other.clear();
		}

		inplace_vector_t& operator=(inplace_vector_t&& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
		{
			if (this == &other)
				return *this;

			clear();
			for (T& value : other)
				emplace_back(std::move(value));
			other.clear();
			return *this;
		}

		~inplace_vector_t()
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
