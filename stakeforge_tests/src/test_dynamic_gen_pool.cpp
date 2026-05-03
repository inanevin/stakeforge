#include "test_registry.hpp"

#include "sfg/memory/dynamic_gen_pool.hpp"
#include "sfg/memory/inplace_gen_pool.hpp"

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

			using allocator_t = sfg::dynamic_gen_pool_t<int>;
			static_assert(!std::is_copy_constructible_v<allocator_t>, "dynamic_gen_pool should not be copy constructible");
			static_assert(!std::is_copy_assignable_v<allocator_t>, "dynamic_gen_pool should not be copy assignable");
			static_assert(std::is_move_constructible_v<allocator_t>, "dynamic_gen_pool should be move constructible");
			static_assert(std::is_move_assignable_v<allocator_t>, "dynamic_gen_pool should be move assignable");

			struct material_t
			{
				int value = 0;
			};

			struct texture_t
			{
				int value = 0;
			};

			struct material_pool_a_tag
			{
			};

			struct material_pool_b_tag
			{
			};

			using material_pool_t = sfg::dynamic_gen_pool_t<material_t>;
			using texture_pool_t  = sfg::dynamic_gen_pool_t<texture_t>;
			static_assert(!std::is_same_v<material_pool_t::handle_t, texture_pool_t::handle_t>, "different pool value types should have different handle types");
			static_assert(!std::is_convertible_v<material_pool_t::handle_t, texture_pool_t::handle_t>, "different pool value type handles should not be convertible");

			using material_pool_a_t = sfg::dynamic_gen_pool_t<material_t, u32, material_pool_a_tag>;
			using material_pool_b_t = sfg::dynamic_gen_pool_t<material_t, u32, material_pool_b_tag>;
			static_assert(!std::is_same_v<material_pool_a_t::handle_t, material_pool_b_t::handle_t>, "different explicit pool tags should have different handle types");
			static_assert(!std::is_convertible_v<material_pool_a_t::handle_t, material_pool_b_t::handle_t>, "different explicit pool tag handles should not be convertible");

			using fixed_material_pool_t = sfg::inplace_gen_pool_t<material_t, u16, 8>;
			using fixed_texture_pool_t	= sfg::inplace_gen_pool_t<texture_t, u16, 8>;
			static_assert(!std::is_same_v<fixed_material_pool_t::handle_t, fixed_texture_pool_t::handle_t>, "fixed pools should have typed handles");

			bool add_remove_and_reuse_preserve_generation_contract()
			{
				test_context_t context;
				context.suite	 = "dynamic_gen_pool";
				context.name	 = "add_remove_and_reuse_preserve_generation_contract";
				context.failures = 0;

				allocator_t allocator;

				const allocator_t::HANDLE id0 = allocator.add();
				const allocator_t::HANDLE id1 = allocator.add();
				allocator.get(id0)			  = 11;
				allocator.get(id1)			  = 22;

				SFG_TEST_EXPECT(context, allocator.size() == 2);
				SFG_TEST_EXPECT(context, allocator.head() == 2);
				SFG_TEST_EXPECT(context, allocator.is_valid(id0));
				SFG_TEST_EXPECT(context, allocator.is_valid(id1));

				allocator.remove(id0);

				SFG_TEST_EXPECT(context, allocator.size() == 1);
				SFG_TEST_EXPECT(context, !allocator.is_valid(id0));
				SFG_TEST_EXPECT(context, allocator.is_valid(id1));
				SFG_TEST_EXPECT(context, allocator.contains_holes());

				const allocator_t::HANDLE reused = allocator.add();
				allocator.get(reused)			 = 33;

				SFG_TEST_EXPECT(context, reused.index == id0.index);
				SFG_TEST_EXPECT(context, reused.generation != id0.generation);
				SFG_TEST_EXPECT(context, allocator.get(reused) == 33);
				SFG_TEST_EXPECT(context, allocator.get(id1) == 22);

				return context.failures == 0;
			}

			bool iteration_skips_removed_slots()
			{
				test_context_t context;
				context.suite	 = "dynamic_gen_pool";
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
				int				   const_sum	   = 0;
				for (int value : const_allocator)
					const_sum += value;

				SFG_TEST_EXPECT(context, const_sum == 10);

				return context.failures == 0;
			}

			bool handle_iteration_skips_removed_slots()
			{
				test_context_t context;
				context.suite	 = "dynamic_gen_pool";
				context.name	 = "handle_iteration_skips_removed_slots";
				context.failures = 0;

				allocator_t allocator;

				const allocator_t::HANDLE id0 = allocator.add();
				const allocator_t::HANDLE id1 = allocator.add();
				const allocator_t::HANDLE id2 = allocator.add();

				allocator.remove(id1);

				u32	 count	   = 0;
				bool found_id0 = false;
				bool found_id1 = false;
				bool found_id2 = false;
				for (auto it = allocator.begin_handle(); it != allocator.end_handle(); ++it)
				{
					const allocator_t::HANDLE handle = *it;
					count++;
					found_id0 = found_id0 || handle == id0;
					found_id1 = found_id1 || handle == id1;
					found_id2 = found_id2 || handle == id2;
				}

				SFG_TEST_EXPECT(context, count == 2);
				SFG_TEST_EXPECT(context, found_id0);
				SFG_TEST_EXPECT(context, !found_id1);
				SFG_TEST_EXPECT(context, found_id2);

				return context.failures == 0;
			}

			bool move_assignment_transfers_ownership()
			{
				test_context_t context;
				context.suite	 = "dynamic_gen_pool";
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
				context.suite	 = "dynamic_gen_pool";
				context.name	 = "supports_non_trivial_move_only_types";
				context.failures = 0;

				non_trivial_t::live_count = 0;
				{
					dynamic_gen_pool_t<non_trivial_t> allocator;
					allocator.reserve(1);

					auto id0				  = allocator.add();
					*allocator.get(id0).value = 7;
					auto id1				  = allocator.add();
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
				context.suite	 = "dynamic_gen_pool";
				context.name	 = "resize_zero_destroys_active_slots_and_invalidates_handles";
				context.failures = 0;

				dynamic_gen_pool_t<non_trivial_t> allocator;
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

		void register_dynamic_gen_pool_tests()
		{
			register_test("dynamic_gen_pool", "add_remove_and_reuse_preserve_generation_contract", &add_remove_and_reuse_preserve_generation_contract);
			register_test("dynamic_gen_pool", "iteration_skips_removed_slots", &iteration_skips_removed_slots);
			register_test("dynamic_gen_pool", "handle_iteration_skips_removed_slots", &handle_iteration_skips_removed_slots);
			register_test("dynamic_gen_pool", "move_assignment_transfers_ownership", &move_assignment_transfers_ownership);
			register_test("dynamic_gen_pool", "supports_non_trivial_move_only_types", &supports_non_trivial_move_only_types);
			register_test("dynamic_gen_pool", "resize_zero_destroys_active_slots_and_invalidates_handles", &resize_zero_destroys_active_slots_and_invalidates_handles);
		}
	}
}
