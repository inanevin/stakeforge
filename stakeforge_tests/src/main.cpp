#include "test_registry.hpp"

namespace sfg
{
	namespace tests
	{
		void register_dynamic_pool_allocator_tests();
		void register_dynamic_pool_allocator_gen_tests();
	}
}

int main()
{
	sfg::tests::register_dynamic_pool_allocator_tests();
	sfg::tests::register_dynamic_pool_allocator_gen_tests();
	return sfg::tests::run_all_tests();
}
