#include "test_registry.hpp"

namespace sfg
{
	namespace tests
	{
		void register_frame_allocator_tests();
		void register_dynamic_gen_pool_tests();
		void register_chunk_allocator_tests();
		void register_reflection_registry_tests();
	}
}

int main()
{
	sfg::tests::register_frame_allocator_tests();
	sfg::tests::register_dynamic_gen_pool_tests();
	sfg::tests::register_chunk_allocator_tests();
	sfg::tests::register_reflection_registry_tests();
	return sfg::tests::run_all_tests();
}
