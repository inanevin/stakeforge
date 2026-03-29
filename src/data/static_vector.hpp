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
#include <utility>
#include <initializer_list>

namespace SFG
{
	template <typename T, int N> class static_vector_t
	{
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
				_data[i] = v;
		}

		constexpr static_vector_t(std::initializer_list<T> ilist)
		{
			_head = 0;
			SFG_ASSERT(ilist.size() <= N && "initializer list too big");
			for (auto&& e : ilist)
				_data[_head++] = e;
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
				return _data[0];
			}
			return _data[index];
		}

		const_reference at(size_type index) const
		{
			if (index >= size())
			{
				SFG_ASSERT(false, "");
				return _data[0];
			}
			return _data[index];
		}

		reference front()
		{
			return _data[0];
		}
		const_reference front() const
		{
			return _data[0];
		}

		reference back()
		{
			return _data[_head - 1];
		}
		const_reference back() const
		{
			return _data[_head - 1];
		}

		iterator_t begin()
		{
			return _data;
		}
		const_iterator begin() const
		{
			return _data;
		}
		const_iterator cbegin() const
		{
			return _data;
		}

		iterator_t end()
		{
			return _data + _head;
		}
		const_iterator end() const
		{
			return _data + _head;
		}
		const_iterator cend() const
		{
			return _data + _head;
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
			SFG_ASSERT(!full());
			_data[_head] = value;
			_head++;
		}

		void push_back(T&& value)
		{
			SFG_ASSERT(!full());
			_data[_head] = std::move(value);
			_head++;
		}

		void pop_back()
		{
			if (!empty())
				--_head;
		}

		void clear()
		{
			_head = 0;
		}

		void resize(size_t sz)
		{
			SFG_ASSERT(sz <= capacity);
			_head = sz;
		}

		// Linear search for value. Returns end() if not found.
		iterator_t find(const T& value)
		{
			for (size_type i = 0; i < _head; ++i)
				if (_data[i] == value)
					return _data + i;
			return end();
		}

		const_iterator find(const T& value) const
		{
			for (size_type i = 0; i < _head; ++i)
				if (_data[i] == value)
					return _data + i;
			return end();
		}

		// Linear search by predicate. Returns end() if not found.
		template <typename Pred> iterator_t find_if(Pred pred)
		{
			for (size_type i = 0; i < _head; ++i)
				if (pred(_data[i]))
					return _data + i;
			return end();
		}

		template <typename Pred> const_iterator find_if(Pred pred) const
		{
			for (size_type i = 0; i < _head; ++i)
				if (pred(_data[i]))
					return _data + i;
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
				_data[i] = _data[i + 1];

			if (idx == _head - 1)
				_data[idx] = T();

			_head--;
		}

		void remove_index_swap(size_t idx)
		{
			SFG_ASSERT(idx < _head);

			if (idx < _head - 1)
				_data[idx] = _data[_head - 1];
			else
				_data[idx] = T();

			_head--;
		}

		T* data()
		{
			return _data;
		}
		const T* data() const
		{
			return _data;
		}

	private:
		T	   _data[N] = {};
		size_t _head	= 0;
	};
}
