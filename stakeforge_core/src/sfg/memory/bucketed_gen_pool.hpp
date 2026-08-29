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
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{

	template <typename T, typename TAG = T> class bucketed_gen_pool_t final
	{

	private:
		struct bucket_t
		{
			T*	 objects	 = nullptr;
			u32* free_list	 = nullptr;
			u32* generations = nullptr;
			u8*	 actives	 = nullptr;
			u32	 free_count	 = 0;
			u8	 used_mark	 = 0;
		};

	public:
		static constexpr u32 BUCKET_SHIFT	 = 24;
		static constexpr u32 GENERATION_MASK = (1 << BUCKET_SHIFT) - 1u; // 00.... 11....
		static constexpr u32 MAX_BUCKETS	 = UINT8_MAX - 1;

		bucketed_gen_pool_t() = default;
		~bucketed_gen_pool_t()
		{
			uninit();
		}

		bucketed_gen_pool_t(const bucketed_gen_pool_t& other)			 = delete;
		bucketed_gen_pool_t& operator=(const bucketed_gen_pool_t& other) = delete;
		bucketed_gen_pool_t& operator=(bucketed_gen_pool_t&& other)		 = delete;

		inline void init(u32 bucket_size)
		{
			SFG_ASSERT(bucket_size != 0 && _buckets == nullptr);

			_bucket_cap		= bucket_size;
			_buckets		= new bucket_t[MAX_BUCKETS];
			_free_buckets	= new u32[MAX_BUCKETS];
			_active_buckets = new u8[MAX_BUCKETS];

			for (u32 i = 0; i < MAX_BUCKETS; i++)
			{
				_active_buckets[i] = 0;
				_free_buckets[i]   = MAX_BUCKETS - i - 1;
			}

			_free_bucket_count = MAX_BUCKETS;
			alloc_bucket();
		}

		inline void uninit()
		{
			if (_buckets == nullptr)
				return;

			for (u32 i = 0; i < MAX_BUCKETS; i++)
			{
				if (_active_buckets[i] == 0)
				{
					delete[] _buckets[i].generations;
					continue;
				}

				free_bucket(i, true);
			}

			delete[] _active_buckets;
			delete[] _free_buckets;
			delete[] _buckets;
			_active_buckets	   = nullptr;
			_buckets		   = nullptr;
			_free_buckets	   = nullptr;
			_free_bucket_count = 0;
			_bucket_cap		   = 0;
		}

		template <typename... Args> inline pool_handle_t<u32, TAG> emplace(Args&&... args)
		{
			bucket_t* bucket	 = nullptr;
			u32		  bucket_idx = UINT32_MAX;

			for (u32 i = 0; i < MAX_BUCKETS; i++)
			{
				if (_active_buckets[i] == 0)
					continue;

				if (_buckets[i].free_count == 0)
					continue;

				bucket	   = &_buckets[i];
				bucket_idx = i;
				break;
			}

			if (bucket == nullptr)
			{
				bucket_idx = alloc_bucket();

				if (bucket_idx == UINT32_MAX)
					return {};

				bucket = &_buckets[bucket_idx];
			}

			const u32 idx = bucket->free_list[--bucket->free_count];

			const u32 generation = bucket->generations[idx] & GENERATION_MASK;

			bucket->generations[idx] = (bucket_idx << BUCKET_SHIFT) | (generation & GENERATION_MASK);
			bucket->actives[idx]	 = 1;

			new (&bucket->objects[idx]) T(std::forward<Args>(args)...);
			return {.generation = bucket->generations[idx], .index = idx};
		}

		inline void remove(pool_handle_t<u32, TAG> handle)
		{
			SFG_ASSERT(is_valid(handle));

			const u32 bucket_idx		 = handle.generation >> BUCKET_SHIFT;
			bucket_t& bucket			 = _buckets[bucket_idx];
			bucket.actives[handle.index] = 0;

			u32 gen = bucket.generations[handle.index] & GENERATION_MASK;
			gen		= (gen + 1) & GENERATION_MASK;
			if (gen == 0)
				gen = 1;

			bucket.generations[handle.index] = (bucket_idx << BUCKET_SHIFT) | gen;

			bucket.free_list[bucket.free_count] = handle.index;
			bucket.free_count++;

			if constexpr (!std::is_trivially_destructible_v<T>)
				std::destroy_at(&bucket.objects[handle.index]);

			if (bucket.free_count == _bucket_cap)
			{
				free_bucket(bucket_idx, false);
			}
		}

		inline bool is_valid(pool_handle_t<u32, TAG> handle) const
		{
			if (handle.is_null())
				return false;

			if (handle.index >= _bucket_cap)
				return false;

			const u32 bucket_idx = handle.generation >> BUCKET_SHIFT;
			if (bucket_idx >= MAX_BUCKETS)
				return false;

			if (_active_buckets[bucket_idx] == 0)
				return false;

			if (_buckets[bucket_idx].actives[handle.index] == 0)
				return false;

			if (_buckets[bucket_idx].generations[handle.index] != handle.generation)
				return false;

			return true;
		}

		inline T& get(pool_handle_t<u32, TAG> handle)
		{
			SFG_ASSERT(is_valid(handle));
			const u32 bucket_idx = handle.generation >> BUCKET_SHIFT;
			return _buckets[bucket_idx].objects[handle.index];
		}

		inline const T& get(pool_handle_t<u32, TAG> handle) const
		{
			SFG_ASSERT(is_valid(handle));
			const u32 bucket_idx = handle.generation >> BUCKET_SHIFT;
			return _buckets[bucket_idx].objects[handle.index];
		}

	private:
		static inline bool find_first_active(u32 bucket_start, u32 slot_start, u32 max_slot, const bucket_t* buckets, const u8* bucket_actives, u32& result_bucket, u32& result_slot)
		{
			u32 slot_search = slot_start;

			for (u32 i = bucket_start; i < MAX_BUCKETS; i++)
			{
				if (bucket_actives[i] == 0)
					continue;

				const bucket_t& bucket = buckets[i];

				for (u32 j = slot_search; j < max_slot; j++)
				{
					if (bucket.actives[j] == 0)
						continue;

					result_bucket = i;
					result_slot	  = j;
					return true;
				}

				slot_search = 0;
			}

			result_bucket = MAX_BUCKETS - 1;
			result_slot	  = max_slot;
			return false;
		}

		inline bool find_begin_position(u32& bucket, u32& slot) const
		{
			if (_buckets == nullptr)
				return false;

			return find_first_active(0, 0, _bucket_cap, _buckets, _active_buckets, bucket, slot);
		}

	public:
		// -----------------------------------------------------------------------------
		// iterator
		// -----------------------------------------------------------------------------

		template <typename OBJECT_TYPE, typename BUCKET_TYPE> struct iterator_impl_t
		{
			using reference = OBJECT_TYPE&;
			using pointer	= OBJECT_TYPE*;

			iterator_impl_t(BUCKET_TYPE* pbuckets, const u8* pactives, u32 cb, u32 c, u32 e) : buckets(pbuckets), active_buckets(pactives), current_bucket(cb), current(c), end(e)
			{
			}

			reference operator*() const
			{
				return buckets[current_bucket].objects[current];
			}

			pointer operator->() const
			{
				return &buckets[current_bucket].objects[current];
			}

			iterator_impl_t& operator++()
			{
				if (!bucketed_gen_pool_t::find_first_active(current_bucket, current + 1, end, buckets, active_buckets, current_bucket, current))
				{
					current		   = end;
					current_bucket = MAX_BUCKETS - 1;
				}

				return *this;
			}

			iterator_impl_t operator++(int)
			{
				iterator_impl_t tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const iterator_impl_t& a, const iterator_impl_t& b)
			{
				return a.current == b.current && a.current_bucket == b.current_bucket;
			}

			friend bool operator!=(const iterator_impl_t& a, const iterator_impl_t& b)
			{
				return a.current != b.current || a.current_bucket != b.current_bucket;
			}

			BUCKET_TYPE* buckets		= nullptr;
			const u8*	 active_buckets = nullptr;
			u32			 current_bucket = 0;
			u32			 current		= 0;
			u32			 end			= 0;
		};

		using iterator_t	   = iterator_impl_t<T, bucket_t>;
		using const_iterator_t = iterator_impl_t<const T, const bucket_t>;

		iterator_t begin()
		{
			u32 start_bucket = 0;
			u32 start_slot	 = 0;

			if (!find_begin_position(start_bucket, start_slot))
				return end();

			return iterator_t(_buckets, _active_buckets, start_bucket, start_slot, _bucket_cap);
		}

		iterator_t end()
		{
			return iterator_t(_buckets, _active_buckets, MAX_BUCKETS - 1, _bucket_cap, _bucket_cap);
		}

		const_iterator_t begin() const
		{
			u32 start_bucket = 0;
			u32 start_slot	 = 0;

			if (!find_begin_position(start_bucket, start_slot))
				return end();

			return const_iterator_t(_buckets, _active_buckets, start_bucket, start_slot, _bucket_cap);
		}

		const_iterator_t end() const
		{
			return const_iterator_t(_buckets, _active_buckets, MAX_BUCKETS - 1, _bucket_cap, _bucket_cap);
		}

		// -----------------------------------------------------------------------------
		// handle iterator
		// -----------------------------------------------------------------------------

		struct handle_iterator_t
		{
			handle_iterator_t(const bucket_t* pbuckets, const u8* pactives, u32 cb, u32 c, u32 e) : buckets(pbuckets), active_buckets(pactives), current_bucket(cb), current(c), end(e)
			{
			}

			pool_handle_t<u32, TAG> operator*() const
			{
				return {
					.generation = buckets[current_bucket].generations[current],
					.index		= current,
				};
			}

			handle_iterator_t& operator++()
			{
				if (!bucketed_gen_pool_t::find_first_active(current_bucket, current + 1, end, buckets, active_buckets, current_bucket, current))
				{
					current		   = end;
					current_bucket = MAX_BUCKETS - 1;
				}

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
				return a.current == b.current && a.current_bucket == b.current_bucket;
			}

			friend bool operator!=(const handle_iterator_t& a, const handle_iterator_t& b)
			{
				return a.current != b.current || a.current_bucket != b.current_bucket;
			}

			const bucket_t* buckets		   = nullptr;
			const u8*		active_buckets = nullptr;
			u32				current_bucket = 0;
			u32				current		   = 0;
			u32				end			   = 0;
		};

		handle_iterator_t begin_handles()
		{
			u32 start_bucket = 0;
			u32 start_slot	 = 0;

			if (!find_begin_position(start_bucket, start_slot))
				return end_handles();

			return handle_iterator_t(_buckets, _active_buckets, start_bucket, start_slot, _bucket_cap);
		}

		handle_iterator_t end_handles()
		{
			return handle_iterator_t(_buckets, _active_buckets, MAX_BUCKETS - 1, _bucket_cap, _bucket_cap);
		}

		handle_iterator_t begin_handles() const
		{
			u32 start_bucket = 0;
			u32 start_slot	 = 0;

			if (!find_begin_position(start_bucket, start_slot))
				return end_handles();

			return handle_iterator_t(_buckets, _active_buckets, start_bucket, start_slot, _bucket_cap);
		}

		handle_iterator_t end_handles() const
		{
			return handle_iterator_t(_buckets, _active_buckets, MAX_BUCKETS - 1, _bucket_cap, _bucket_cap);
		}

	private:
		inline u32 alloc_bucket()
		{
			if (_free_bucket_count == 0)
			{
				SFG_ASSERT(false);
				return UINT32_MAX;
			}

			void* ptr = SFG_ALIGNED_MALLOC(alignof(T), sizeof(T) * _bucket_cap);
			if (ptr == nullptr)
				return UINT32_MAX;

			const u32 free_idx = _free_bucket_count - 1;
			_free_bucket_count--;

			const u32 idx		 = _free_buckets[free_idx];
			_active_buckets[idx] = 1;

			bucket_t& b = _buckets[idx];

			b.objects	= reinterpret_cast<T*>(ptr);
			b.free_list = new u32[_bucket_cap];
			if (b.used_mark == 0)
				b.generations = new u32[_bucket_cap];
			b.actives	 = new u8[_bucket_cap];
			b.free_count = _bucket_cap;

			for (u32 i = 0; i < _bucket_cap; i++)
			{
				if (b.used_mark == 0)
					b.generations[i] = (static_cast<u32>(UINT8_MAX) << BUCKET_SHIFT) | (static_cast<u32>(1) & GENERATION_MASK);
				b.actives[i]   = 0;
				b.free_list[i] = _bucket_cap - i - 1;
			}

			b.used_mark = 1;
			return idx;
		}

		inline void free_bucket(u32 idx, bool teardown)
		{
			_free_buckets[_free_bucket_count] = idx;
			_free_bucket_count++;
			_active_buckets[idx] = 0;

			bucket_t& b = _buckets[idx];

			if (teardown)
			{
				for (u32 i = 0; i < _bucket_cap; i++)
				{
					if (b.actives[i] == 0)
						continue;

					if constexpr (!std::is_trivially_destructible_v<T>)
						std::destroy_at(&b.objects[i]);
				}
			}

			SFG_ALIGNED_FREE(b.objects);
			delete[] b.free_list;
			delete[] b.actives;
			b.objects	= nullptr;
			b.actives	= nullptr;
			b.free_list = nullptr;

			if (teardown)
			{
				delete[] b.generations;
				b.generations = nullptr;
			}
		}

	private:
		bucket_t* _buckets			 = nullptr;
		u32*	  _free_buckets		 = nullptr;
		u32		  _free_bucket_count = 0;
		u8*		  _active_buckets	 = nullptr;
		u32		  _bucket_cap		 = 0;
	};

}
