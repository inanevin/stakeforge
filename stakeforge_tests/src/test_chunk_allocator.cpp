#include "test_registry.hpp"

#include "sfg/memory/chunk_allocator.hpp"

namespace sfg
{
	namespace tests
	{
		namespace
		{
			bool fixed_capacity_does_not_grow()
			{
				test_context_t context;
				context.suite	 = "chunk_allocator";
				context.name	 = "fixed_capacity_does_not_grow";
				context.failures = 0;

				chunk_allocator_t allocator;
				allocator.init(128);

				const u32 capacity = allocator.get_capacity();

				const chunk_handle32_t first = allocator.allocate<u32>(2);
				u32*				   data	 = allocator.get<u32>(first);
				data[0]						 = 11;
				data[1]						 = 22;

				const chunk_handle32_t second = allocator.allocate_bytes(64, alignof(u32));

				SFG_TEST_EXPECT(context, allocator.get_capacity() == capacity);
				SFG_TEST_EXPECT(context, first.head == 0);
				SFG_TEST_EXPECT(context, allocator.get<u32>(first)[0] == 11);
				SFG_TEST_EXPECT(context, allocator.get<u32>(first)[1] == 22);
				SFG_TEST_EXPECT(context, second.size == 64);

				allocator.uninit();
				return context.failures == 0;
			}

			bool free_chunks_are_reused()
			{
				test_context_t context;
				context.suite	 = "chunk_allocator";
				context.name	 = "free_chunks_are_reused";
				context.failures = 0;

				chunk_allocator_t allocator;
				allocator.init(64);

				const chunk_handle32_t first  = allocator.allocate<u32>(4);
				const chunk_handle32_t second = allocator.allocate<u32>(4);
				allocator.free(first);

				const chunk_handle32_t reused = allocator.allocate<u32>(2);

				SFG_TEST_EXPECT(context, reused.head == first.head);
				SFG_TEST_EXPECT(context, reused.size == sizeof(u32) * 2);
				SFG_TEST_EXPECT(context, second.head != reused.head);

				allocator.uninit();
				return context.failures == 0;
			}
		}

		void register_chunk_allocator_tests()
		{
			register_test("chunk_allocator", "fixed_capacity_does_not_grow", &fixed_capacity_does_not_grow);
			register_test("chunk_allocator", "free_chunks_are_reused", &free_chunks_are_reused);
		}
	}
}
