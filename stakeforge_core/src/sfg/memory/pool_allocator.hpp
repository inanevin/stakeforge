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

#include "common/size_definitions.hpp"
#include "io/assert.hpp"

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace sfg
{

	template <typename T, typename U, int N> struct pool_allocator_t
	{
		inline U add()
		{
			return emplace();
		}

		template <typename... Args> inline U emplace(Args&&... args)
		{
			if (_free_size != 0)
			{
				const U id = _free_list[_free_size - 1];
				_free_size--;
				std::construct_at(ptr(id), std::forward<Args>(args)...);
				_actives[id] = 1;
				_size++;
				return id;
			}

			const U id = _head;
			SFG_ASSERT(id < N);
			_head++;
			std::construct_at(ptr(id), std::forward<Args>(args)...);
			_actives[id] = 1;
			_size++;
			return id;
		}

		void remove(U id)
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			_free_list[_free_size] = id;
			_free_size++;
			std::destroy_at(ptr(id));
			_actives[id] = 0;
			_size--;
		}

		T& get(U id)
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			return *ptr(id);
		}

		const T& get(U id) const
		{
			SFG_ASSERT(id < _head && _actives[id] != 0);
			return *ptr(id);
		}

		inline void verify_uninit()
		{
			SFG_ASSERT(_size == 0);
		}

		void reset()
		{
			clear_active();
			_free_size = 0;
			_head	   = 0;
		}

		~pool_allocator_t()
		{
			clear_active();
		}

	private:
		struct storage_t
		{
			alignas(T) unsigned char bytes[sizeof(T)];
		};

		T* ptr(U id)
		{
			return std::launder(reinterpret_cast<T*>(&_data[id]));
		}

		const T* ptr(U id) const
		{
			return std::launder(reinterpret_cast<const T*>(&_data[id]));
		}

		void clear_active()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (U i = 0; i < _head; i++)
				{
					if (_actives[i] != 0)
						std::destroy_at(ptr(i));
				}
			}

			for (U i = 0; i < _head; i++)
				_actives[i] = 0;

			_size = 0;
		}

	private:
		storage_t _data[N]		= {};
		U		  _free_list[N] = {};
		u8		  _actives[N]	= {};
		U		  _head			= 0;
		U		  _free_size	= 0;
		U		  _size			= 0;
	};

}
