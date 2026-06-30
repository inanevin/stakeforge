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

#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	class reflection_container_ops_t
	{
	public:
		template <typename T> static inline u8* vector_get_element_ptr(void* obj, u32 index)
		{
			vector_t<T>& values = *reinterpret_cast<vector_t<T>*>(obj);
			return reinterpret_cast<u8*>(&values[index]);
		}

		template <typename T> static inline u8* vector_add_element_ptr(void* obj)
		{
			vector_t<T>& values = *reinterpret_cast<vector_t<T>*>(obj);
			values.push_back(T{});
			return reinterpret_cast<u8*>(&values.back());
		}

		template <typename T> static inline size_t vector_get_element_size(void* obj)
		{
			vector_t<T>& values = *reinterpret_cast<vector_t<T>*>(obj);
			return values.size();
		}

		template <typename T> static inline void vector_reset(void* obj)
		{
			vector_t<T>& values = *reinterpret_cast<vector_t<T>*>(obj);
			values.resize(0);
		}

		template <typename T, int N> static inline u8* inplace_vector_get_element_ptr(void* obj, u32 index)
		{
			inplace_vector_t<T, N>& values = *reinterpret_cast<inplace_vector_t<T, N>*>(obj);
			return reinterpret_cast<u8*>(&values[index]);
		}

		template <typename T, int N> static inline u8* inplace_vector_add_element_ptr(void* obj)
		{
			inplace_vector_t<T, N>& values = *reinterpret_cast<inplace_vector_t<T, N>*>(obj);
			if (values.full())
				return nullptr;

			values.push_back(T{});
			return reinterpret_cast<u8*>(&values.back());
		}

		template <typename T, int N> static inline size_t inplace_vector_get_element_size(void* obj)
		{
			inplace_vector_t<T, N>& values = *reinterpret_cast<inplace_vector_t<T, N>*>(obj);
			return values.size();
		}

		template <typename T, int N> static inline void inplace_vector_reset(void* obj)
		{
			inplace_vector_t<T, N>& values = *reinterpret_cast<inplace_vector_t<T, N>*>(obj);
			values.resize(0);
		}

		template <typename TContainer, typename T, size_t N, T (TContainer::*Data)[N], size_t TContainer::* Size> static inline u8* sized_array_get_element_ptr(void* obj, u32 index)
		{
			TContainer& values = *reinterpret_cast<TContainer*>(obj);
			return reinterpret_cast<u8*>(&(values.*Data)[index]);
		}

		template <typename TContainer, typename T, size_t N, T (TContainer::*Data)[N], size_t TContainer::* Size> static inline u8* sized_array_add_element_ptr(void* obj)
		{
			TContainer& values = *reinterpret_cast<TContainer*>(obj);
			if (values.*Size == N)
				return nullptr;

			(values.*Data)[values.*Size] = T{};
			return reinterpret_cast<u8*>(&(values.*Data)[(values.*Size)++]);
		}

		template <typename TContainer, typename T, size_t N, T (TContainer::*Data)[N], size_t TContainer::* Size> static inline size_t sized_array_get_element_size(void* obj)
		{
			TContainer& values = *reinterpret_cast<TContainer*>(obj);
			return values.*Size;
		}

		template <typename TContainer, typename T, size_t N, T (TContainer::*Data)[N], size_t TContainer::* Size> static inline void sized_array_reset(void* obj)
		{
			TContainer& values = *reinterpret_cast<TContainer*>(obj);
			values.*Size	   = 0;
		}

		template <typename T> static inline reflected_field_container_ops_t vector_ops(reflected_value_type_e value_type, sid_t sub_type_id = 0)
		{
			return {
				.add_element_ptr_fn	 = &vector_add_element_ptr<T>,
				.reset_fn			 = &vector_reset<T>,
				.get_element_ptr_fn	 = &vector_get_element_ptr<T>,
				.get_element_size_fn = &vector_get_element_size<T>,
				.element_value_type	 = value_type,
				.element_sub_type_id = sub_type_id,
				.element_value_size	 = sizeof(T),
			};
		}

		template <typename T, int N> static inline reflected_field_container_ops_t inplace_vector_ops(reflected_value_type_e value_type, sid_t sub_type_id = 0)
		{
			return {
				.add_element_ptr_fn	 = &inplace_vector_add_element_ptr<T, N>,
				.reset_fn			 = &inplace_vector_reset<T, N>,
				.get_element_ptr_fn	 = &inplace_vector_get_element_ptr<T, N>,
				.get_element_size_fn = &inplace_vector_get_element_size<T, N>,
				.element_value_type	 = value_type,
				.element_sub_type_id = sub_type_id,
				.element_value_size	 = sizeof(T),
			};
		}

		template <typename TContainer, typename T, size_t N, T (TContainer::*Data)[N], size_t TContainer::* Size> static inline reflected_field_container_ops_t sized_array_ops(reflected_value_type_e value_type, sid_t sub_type_id = 0)
		{
			return {
				.add_element_ptr_fn	 = &sized_array_add_element_ptr<TContainer, T, N, Data, Size>,
				.reset_fn			 = &sized_array_reset<TContainer, T, N, Data, Size>,
				.get_element_ptr_fn	 = &sized_array_get_element_ptr<TContainer, T, N, Data, Size>,
				.get_element_size_fn = &sized_array_get_element_size<TContainer, T, N, Data, Size>,
				.element_value_type	 = value_type,
				.element_sub_type_id = sub_type_id,
				.element_value_size	 = sizeof(T),
			};
		}
	};
}
