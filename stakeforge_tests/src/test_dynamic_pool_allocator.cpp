#include "test_registry.hpp"

#include "sfg/memory/dynamic_pool_allocator.hpp"

#include <type_traits>

namespace sfg
{
	namespace tests
	{
		namespace
		{
			using allocator_t = sfg::dynamic_pool_allocator_t<int>;
			static_assert(!std::is_copy_constructible<allocator_t>::value, "dynamic_pool_allocator should not be copy constructible");

			bool add_returns_sequential_indices()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator";
				context.name	 = "add_returns_sequential_indices";
				context.failures = 0;

				allocator_t allocator;

				const u32 id0 = allocator.add();
				const u32 id1 = allocator.add();
				const u32 id2 = allocator.add();

				SFG_TEST_EXPECT(context, id0 == 0);
				SFG_TEST_EXPECT(context, id1 == 1);
				SFG_TEST_EXPECT(context, id2 == 2);
				SFG_TEST_EXPECT(context, allocator.size() == 3);
				SFG_TEST_EXPECT(context, allocator.capacity() >= allocator.size());

				return context.failures == 0;
			}

			bool remove_reuses_free_index()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator";
				context.name	 = "remove_reuses_free_index";
				context.failures = 0;

				allocator_t allocator;

				u32 id0 = allocator.add();
				u32 id1 = allocator.add();
				u32 id2 = allocator.add();

				allocator.get(id0) = 11;
				allocator.get(id1) = 22;
				allocator.get(id2) = 33;

				const bool removed	 = allocator.remove(id1);
				const u32  reused_id = allocator.add();

				SFG_TEST_EXPECT(context, removed);
				SFG_TEST_EXPECT(context, reused_id == id1);
				SFG_TEST_EXPECT(context, allocator.get(id0) == 11);
				SFG_TEST_EXPECT(context, allocator.get(id2) == 33);

				return context.failures == 0;
			}

			bool reserve_and_clear_update_allocator_state()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator";
				context.name	 = "reserve_and_clear_update_allocator_state";
				context.failures = 0;

				allocator_t allocator;

				allocator.reserve(32);
				SFG_TEST_EXPECT(context, allocator.capacity() >= 32);
				SFG_TEST_EXPECT(context, allocator.size() == 0);

				allocator.add();
				allocator.add();
				allocator.clear();

				SFG_TEST_EXPECT(context, allocator.capacity() == 0);
				SFG_TEST_EXPECT(context, allocator.size() == 0);

				const u32 id = allocator.add();
				SFG_TEST_EXPECT(context, id == 0);
				SFG_TEST_EXPECT(context, allocator.size() == 1);

				return context.failures == 0;
			}

			bool assignment_transfers_allocator_ownership()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator";
				context.name	 = "assignment_transfers_allocator_ownership";
				context.failures = 0;

				allocator_t source;
				allocator_t target;

				const u32 id0	= source.add();
				const u32 id1	= source.add();
				source.get(id0) = 17;
				source.get(id1) = 29;

				target = source;

				SFG_TEST_EXPECT(context, source.size() == 0);
				SFG_TEST_EXPECT(context, source.capacity() == 0);
				SFG_TEST_EXPECT(context, target.size() == 2);
				SFG_TEST_EXPECT(context, target.capacity() >= 2);
				SFG_TEST_EXPECT(context, target.get(id0) == 17);
				SFG_TEST_EXPECT(context, target.get(id1) == 29);

				const u32 id2 = target.add();
				SFG_TEST_EXPECT(context, id2 == 2);
				SFG_TEST_EXPECT(context, target.size() == 3);

				return context.failures == 0;
			}
		}

		void register_dynamic_pool_allocator_tests()
		{
			register_test("dynamic_pool_allocator", "add_returns_sequential_indices", &add_returns_sequential_indices);
			register_test("dynamic_pool_allocator", "remove_reuses_free_index", &remove_reuses_free_index);
			register_test("dynamic_pool_allocator", "reserve_and_clear_update_allocator_state", &reserve_and_clear_update_allocator_state);
			register_test("dynamic_pool_allocator", "assignment_transfers_allocator_ownership", &assignment_transfers_allocator_ownership);
		}
	}
}
