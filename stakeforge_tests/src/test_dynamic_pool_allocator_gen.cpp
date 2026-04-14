#include "test_registry.hpp"

#include "sfg/memory/dynamic_pool_allocator_gen.hpp"

#include <memory>
#include <type_traits>

namespace sfg
{
	namespace tests
	{
		namespace
		{
			struct non_trivial_t
			{
				non_trivial_t() : value(std::make_unique<int>(0))
				{
					live_count++;
				}

				~non_trivial_t()
				{
					live_count--;
				}

				non_trivial_t(const non_trivial_t&)			   = delete;
				non_trivial_t& operator=(const non_trivial_t&) = delete;

				non_trivial_t(non_trivial_t&& other) noexcept : value(std::move(other.value))
				{
					live_count++;
				}

				non_trivial_t& operator=(non_trivial_t&& other) noexcept
				{
					value = std::move(other.value);
					return *this;
				}

				std::unique_ptr<int> value;

				static inline int live_count = 0;
			};

			using allocator_t = sfg::dynamic_pool_allocator_gen_t<int>;
			static_assert(!std::is_copy_constructible_v<allocator_t>, "dynamic_pool_allocator_gen should not be copy constructible");
			static_assert(!std::is_copy_assignable_v<allocator_t>, "dynamic_pool_allocator_gen should not be copy assignable");
			static_assert(std::is_move_constructible_v<allocator_t>, "dynamic_pool_allocator_gen should be move constructible");
			static_assert(std::is_move_assignable_v<allocator_t>, "dynamic_pool_allocator_gen should be move assignable");

			bool add_remove_and_reuse_preserve_generation_contract()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator_gen";
				context.name	 = "add_remove_and_reuse_preserve_generation_contract";
				context.failures = 0;

				allocator_t allocator;

				const allocator_t::HANDLE id0 = allocator.add();
				const allocator_t::HANDLE id1 = allocator.add();
				allocator.get(id0)			  = 11;
				allocator.get(id1)			  = 22;

				SFG_TEST_EXPECT(context, allocator.size() == 2);
				SFG_TEST_EXPECT(context, allocator.high_water_mark() == 2);
				SFG_TEST_EXPECT(context, allocator.is_valid(id0));
				SFG_TEST_EXPECT(context, allocator.is_valid(id1));

				allocator.remove(id0);

				SFG_TEST_EXPECT(context, allocator.size() == 1);
				SFG_TEST_EXPECT(context, !allocator.is_valid(id0));
				SFG_TEST_EXPECT(context, allocator.is_valid(id1));
				SFG_TEST_EXPECT(context, allocator.contains_holes());

				const allocator_t::HANDLE reused = allocator.add();
				allocator.get(reused)			   = 33;

				SFG_TEST_EXPECT(context, reused.index == id0.index);
				SFG_TEST_EXPECT(context, reused.generation != id0.generation);
				SFG_TEST_EXPECT(context, allocator.get(reused) == 33);
				SFG_TEST_EXPECT(context, allocator.get(id1) == 22);

				return context.failures == 0;
			}

			bool iteration_skips_removed_slots()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator_gen";
				context.name	 = "iteration_skips_removed_slots";
				context.failures = 0;

				allocator_t allocator;

				const allocator_t::HANDLE id0 = allocator.add();
				const allocator_t::HANDLE id1 = allocator.add();
				const allocator_t::HANDLE id2 = allocator.add();
				allocator.get(id0)			  = 2;
				allocator.get(id1)			  = 4;
				allocator.get(id2)			  = 8;

				allocator.remove(id1);

				int sum = 0;
				for (int value : allocator)
					sum += value;

				SFG_TEST_EXPECT(context, sum == 10);

				const allocator_t& const_allocator = allocator;
				int				  const_sum		  = 0;
				for (int value : const_allocator)
					const_sum += value;

				SFG_TEST_EXPECT(context, const_sum == 10);

				return context.failures == 0;
			}

			bool move_assignment_transfers_ownership()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator_gen";
				context.name	 = "move_assignment_transfers_ownership";
				context.failures = 0;

				allocator_t source;
				allocator_t target;

				const allocator_t::HANDLE id0 = source.add();
				source.get(id0)				  = 17;

				target = std::move(source);

				SFG_TEST_EXPECT(context, source.size() == 0);
				SFG_TEST_EXPECT(context, source.capacity() == 0);
				SFG_TEST_EXPECT(context, target.size() == 1);
				SFG_TEST_EXPECT(context, target.get(id0) == 17);

				return context.failures == 0;
			}

			bool supports_non_trivial_move_only_types()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator_gen";
				context.name	 = "supports_non_trivial_move_only_types";
				context.failures = 0;

				non_trivial_t::live_count = 0;
				{
					dynamic_pool_allocator_gen_t<non_trivial_t> allocator;
					allocator.reserve(1);

					auto id0				 = allocator.add();
					*allocator.get(id0).value = 7;
					auto id1				 = allocator.add();
					*allocator.get(id1).value = 13;

					SFG_TEST_EXPECT(context, allocator.capacity() >= 2);
					SFG_TEST_EXPECT(context, allocator.size() == 2);
					SFG_TEST_EXPECT(context, *allocator.get(id0).value == 7);
					SFG_TEST_EXPECT(context, *allocator.get(id1).value == 13);
					SFG_TEST_EXPECT(context, non_trivial_t::live_count == 2);

					allocator.remove(id0);
					SFG_TEST_EXPECT(context, non_trivial_t::live_count == 1);
				}

				SFG_TEST_EXPECT(context, non_trivial_t::live_count == 0);

				return context.failures == 0;
			}

			bool resize_zero_destroys_active_slots_and_invalidates_handles()
			{
				test_context_t context;
				context.suite	 = "dynamic_pool_allocator_gen";
				context.name	 = "resize_zero_destroys_active_slots_and_invalidates_handles";
				context.failures = 0;

				dynamic_pool_allocator_gen_t<non_trivial_t> allocator;
				non_trivial_t::live_count = 0;

				auto id0 = allocator.add();
				auto id1 = allocator.add();

				SFG_TEST_EXPECT(context, non_trivial_t::live_count == 2);
				allocator.resize_zero();
				SFG_TEST_EXPECT(context, non_trivial_t::live_count == 0);
				SFG_TEST_EXPECT(context, allocator.size() == 0);
				SFG_TEST_EXPECT(context, !allocator.is_valid(id0));
				SFG_TEST_EXPECT(context, !allocator.is_valid(id1));

				auto id2 = allocator.add();
				SFG_TEST_EXPECT(context, id2.index == 0);
				SFG_TEST_EXPECT(context, id2.generation != id0.generation);

				return context.failures == 0;
			}
		}

		void register_dynamic_pool_allocator_gen_tests()
		{
			register_test("dynamic_pool_allocator_gen", "add_remove_and_reuse_preserve_generation_contract", &add_remove_and_reuse_preserve_generation_contract);
			register_test("dynamic_pool_allocator_gen", "iteration_skips_removed_slots", &iteration_skips_removed_slots);
			register_test("dynamic_pool_allocator_gen", "move_assignment_transfers_ownership", &move_assignment_transfers_ownership);
			register_test("dynamic_pool_allocator_gen", "supports_non_trivial_move_only_types", &supports_non_trivial_move_only_types);
			register_test("dynamic_pool_allocator_gen", "resize_zero_destroys_active_slots_and_invalidates_handles", &resize_zero_destroys_active_slots_and_invalidates_handles);
		}
	}
}
