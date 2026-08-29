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

#include "test_registry.hpp"

#include <sfg/memory/bucketed_gen_pool.hpp>

namespace sfg
{
	namespace tests
	{
		namespace
		{
			struct bucketed_item_t
			{
				i32 value = 0;
			};

			using bucketed_pool_t = bucketed_gen_pool_t<bucketed_item_t>;

			bool iterators_cover_sparse_buckets()
			{
				test_context_t context{
					.suite	  = "bucketed_gen_pool",
					.name	  = "iterators_cover_sparse_buckets",
					.failures = 0,
				};

				bucketed_pool_t pool{};
				pool.init(2);

				const auto handle0		= pool.emplace();
				const auto handle1		= pool.emplace();
				const auto handle2		= pool.emplace();
				const auto handle3		= pool.emplace();
				const auto handle4		= pool.emplace();
				pool.get(handle0).value = 1;
				pool.get(handle1).value = 2;
				pool.get(handle2).value = 3;
				pool.get(handle3).value = 4;
				pool.get(handle4).value = 5;

				pool.remove(handle1);
				pool.remove(handle2);
				pool.remove(handle3);

				u32 object_count = 0;
				i32 object_sum	 = 0;

				for (bucketed_item_t& item : pool)
				{
					object_count++;
					object_sum += item.value;
					item.value += 10;
				}

				SFG_TEST_EXPECT(context, object_count == 2);
				SFG_TEST_EXPECT(context, object_sum == 6);

				const bucketed_pool_t& const_pool  = pool;
				u32					   const_count = 0;
				i32					   const_sum   = 0;

				for (const bucketed_item_t& item : const_pool)
				{
					const_count++;
					const_sum += item.value;
				}

				SFG_TEST_EXPECT(context, const_count == 2);
				SFG_TEST_EXPECT(context, const_sum == 26);

				u32	 mutable_handle_count = 0;
				bool found_handle0		  = false;
				bool found_handle4		  = false;

				for (auto it = pool.begin_handles(); it != pool.end_handles(); ++it)
				{
					const auto handle = *it;
					mutable_handle_count++;
					found_handle0 = found_handle0 || handle == handle0;
					found_handle4 = found_handle4 || handle == handle4;
					SFG_TEST_EXPECT(context, pool.is_valid(handle));
				}

				SFG_TEST_EXPECT(context, mutable_handle_count == 2);
				SFG_TEST_EXPECT(context, found_handle0);
				SFG_TEST_EXPECT(context, found_handle4);

				u32 const_handle_count = 0;

				for (auto it = const_pool.begin_handles(); it != const_pool.end_handles(); ++it)
				{
					const auto handle = *it;
					const_handle_count++;
					SFG_TEST_EXPECT(context, const_pool.is_valid(handle));
				}

				SFG_TEST_EXPECT(context, const_handle_count == 2);
				return context.failures == 0;
			}

			bool empty_iterators_equal_end()
			{
				test_context_t context{
					.suite	  = "bucketed_gen_pool",
					.name	  = "empty_iterators_equal_end",
					.failures = 0,
				};

				bucketed_pool_t pool{};

				SFG_TEST_EXPECT(context, pool.begin() == pool.end());
				SFG_TEST_EXPECT(context, pool.begin_handles() == pool.end_handles());

				pool.init(2);

				const bucketed_pool_t& const_pool = pool;
				SFG_TEST_EXPECT(context, pool.begin() == pool.end());
				SFG_TEST_EXPECT(context, const_pool.begin() == const_pool.end());
				SFG_TEST_EXPECT(context, pool.begin_handles() == pool.end_handles());
				SFG_TEST_EXPECT(context, const_pool.begin_handles() == const_pool.end_handles());

				return context.failures == 0;
			}
		}

		void register_bucketed_gen_pool_tests()
		{
			register_test("bucketed_gen_pool", "iterators_cover_sparse_buckets", &iterators_cover_sparse_buckets);
			register_test("bucketed_gen_pool", "empty_iterators_equal_end", &empty_iterators_equal_end);
		}
	}
}
