#include "test_registry.hpp"

#include "sfg/data/string.hpp"
#include "sfg/data/vector.hpp"
#include "sfg/data/frame_hash_map.hpp"
#include "sfg/data/frame_vector.hpp"
#include "sfg/data/frame_string.hpp"
#include "sfg/memory/frame_allocator.hpp"

#include <thread>

namespace sfg
{
	namespace tests
	{
		namespace
		{
			bool vector_uses_frame_storage()
			{
				test_context_t context;
				context.suite	 = "frame_allocator";
				context.name	 = "vector_uses_frame_storage";
				context.failures = 0;

				frame_allocator_tls_t::init(1024);
				SFG_TEST_EXPECT(context, frame_allocator_tls_t::is_init());
				SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() == 0);

				{
					frame_vector_t<int> values;
					values.reserve(8);
					values.push_back(11);
					values.push_back(22);
					values.push_back(33);

					SFG_TEST_EXPECT(context, values.size() == 3);
					SFG_TEST_EXPECT(context, values[0] == 11);
					SFG_TEST_EXPECT(context, values[1] == 22);
					SFG_TEST_EXPECT(context, values[2] == 33);
					SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() >= sizeof(int) * 8);
				}

				frame_allocator_tls_t::uninit();
				SFG_TEST_EXPECT(context, !frame_allocator_tls_t::is_init());

				return context.failures == 0;
			}

			bool reset_reuses_frame_storage()
			{
				test_context_t context;
				context.suite	 = "frame_allocator";
				context.name	 = "reset_reuses_frame_storage";
				context.failures = 0;

				frame_allocator_tls_t::init(1024);

				size_t first_head = 0;
				{
					frame_vector_t<int> values;
					values.reserve(16);
					first_head = frame_allocator_tls_t::get_head();
					SFG_TEST_EXPECT(context, first_head >= sizeof(int) * 16);
				}

				frame_allocator_tls_t::reset();
				SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() == 0);

				{
					frame_vector_t<int> values;
					values.reserve(16);
					SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() == first_head);
				}

				frame_allocator_tls_t::uninit();

				return context.failures == 0;
			}

			bool string_uses_frame_storage()
			{
				test_context_t context;
				context.suite	 = "frame_allocator";
				context.name	 = "string_uses_frame_storage";
				context.failures = 0;

				frame_allocator_tls_t::init(2048);

				{
					frame_string_t text = "";
					text.assign(128, 'a');

					SFG_TEST_EXPECT(context, text.size() == 128);
					SFG_TEST_EXPECT(context, text[0] == 'a');
					SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() >= 128);
				}

				frame_allocator_tls_t::uninit();

				return context.failures == 0;
			}

			bool hash_map_uses_frame_storage()
			{
				test_context_t context;
				context.suite	 = "frame_allocator";
				context.name	 = "hash_map_uses_frame_storage";
				context.failures = 0;

				frame_allocator_tls_t::init(4096);

				{
					frame_hash_map_t<int, int> values;
					values.reserve(8);
					values.emplace(1, 11);
					values.emplace(2, 22);
					values.emplace(3, 33);

					auto found = values.find(2);
					SFG_TEST_EXPECT(context, values.size() == 3);
					SFG_TEST_EXPECT(context, found != values.end());
					SFG_TEST_EXPECT(context, found->second == 22);
					SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() > 0);
				}

				frame_allocator_tls_t::uninit();

				return context.failures == 0;
			}

			bool thread_local_states_are_independent()
			{
				test_context_t context;
				context.suite	 = "frame_allocator";
				context.name	 = "thread_local_states_are_independent";
				context.failures = 0;

				frame_allocator_tls_t::init(1024);

				size_t main_head = 0;
				{
					frame_vector_t<int> main_values;
					main_values.reserve(4);
					main_head = frame_allocator_tls_t::get_head();
				}

				size_t		thread_head_after_alloc = 0;
				bool		thread_was_init_before	= true;
				std::thread worker([&thread_head_after_alloc, &thread_was_init_before]() {
					thread_was_init_before = frame_allocator_tls_t::is_init();
					frame_allocator_tls_t::init(1024);
					{
						frame_vector_t<int> thread_values;
						thread_values.reserve(8);
						thread_head_after_alloc = frame_allocator_tls_t::get_head();
					}
					frame_allocator_tls_t::uninit();
				});
				worker.join();

				SFG_TEST_EXPECT(context, !thread_was_init_before);
				SFG_TEST_EXPECT(context, thread_head_after_alloc >= sizeof(int) * 8);
				SFG_TEST_EXPECT(context, frame_allocator_tls_t::is_init());
				SFG_TEST_EXPECT(context, frame_allocator_tls_t::get_head() == main_head);

				frame_allocator_tls_t::uninit();

				return context.failures == 0;
			}
		}

		void register_frame_allocator_tests()
		{
			register_test("frame_allocator", "vector_uses_frame_storage", &vector_uses_frame_storage);
			register_test("frame_allocator", "reset_reuses_frame_storage", &reset_reuses_frame_storage);
			register_test("frame_allocator", "string_uses_frame_storage", &string_uses_frame_storage);
			register_test("frame_allocator", "hash_map_uses_frame_storage", &hash_map_uses_frame_storage);
			register_test("frame_allocator", "thread_local_states_are_independent", &thread_local_states_are_independent);
		}
	}
}
