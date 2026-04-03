#pragma once

#include "sfg/io/log.hpp"

namespace sfg
{
	namespace tests
	{
		struct test_context_t
		{
			const char* suite;
			const char* name;
			int			failures;
		};

		using test_function_t = bool (*)();

		struct test_case_t
		{
			const char*		suite;
			const char*		name;
			test_function_t function;
		};

		void register_test(const char* suite, const char* name, test_function_t function);
		void report_expect_failure(test_context_t& context, const char* expression, const char* file, int line);
		int	 run_all_tests();
	}
}

#define SFG_TEST_EXPECT(context, expression)                                                                                                                                                                                                                       \
	do                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                              \
		if (!(expression))                                                                                                                                                                                                                                         \
			sfg::tests::report_expect_failure(context, #expression, __FILE__, __LINE__);                                                                                                                                                                           \
	} while (false)
