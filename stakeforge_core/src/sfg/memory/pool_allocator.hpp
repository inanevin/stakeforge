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

namespace sfg
{

	template <typename T, typename U, int N> struct pool_allocator_t
	{

		inline U add()
		{
			if (_free_size != 0)
			{
				const U id = _free_list[_free_size - 1];
				_free_size--;
				_data[id] = T();
				return id;
			}

			const U id = _head;
			_head++;
			SFG_ASSERT(_head < N);
			_data[id] = T();
			return id;
		}

		void remove(U id)
		{
			_free_list[_free_size] = id;
			_free_size++;
			_data[id].~T();
		}

		T& get(U id)
		{
			SFG_ASSERT(id < N);
			return _data[id];
		}

		const T& get(U id) const
		{
			SFG_ASSERT(id < N);
			return _data[id];
		}

		inline void verify_uninit()
		{
			SFG_ASSERT(_free_size == _head);
		}

		void reset()
		{
			for (int i = 0; i < N; i++)
				_data[i] = T();

			_free_size = 0;
			_head	   = 0;
		}

	private:
		T _data[N]		= {};
		U _free_list[N] = {};
		U _head			= 0;
		U _free_size	= 0;
	};

}
