#include "test_registry.hpp"

namespace sfg
{
	namespace tests
	{
		void register_dynamic_pool_allocator_tests();
	}
}

int main()
{
	sfg::tests::register_dynamic_pool_allocator_tests();
	return sfg::tests::run_all_tests();
}
