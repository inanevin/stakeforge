#include "test_registry.hpp"

#include <vector>

namespace sfg
{
	namespace tests
	{
		static std::vector<test_case_t>& registry()
		{
			static std::vector<test_case_t> tests;
			return tests;
		}

		void register_test(const char* suite, const char* name, test_function_t function)
		{
			test_case_t test;
			test.suite	  = suite;
			test.name	  = name;
			test.function = function;
			registry().push_back(test);
		}

		void report_expect_failure(test_context_t& context, const char* expression, const char* file, int line)
		{
			context.failures++;
			SFG_ERR("[{0}::{1}] expectation failed: {2} ({3}:{4})", context.suite, context.name, expression, file, line);
		}

		int run_all_tests()
		{
			const std::vector<test_case_t>& tests = registry();

			if (tests.empty())
			{
				SFG_WARN("No tests were registered.");
				return 0;
			}

			int passed = 0;
			int failed = 0;

			SFG_TRACE("Running {0} test(s).", tests.size());

			for (size_t i = 0; i < tests.size(); i++)
			{
				const test_case_t& test = tests[i];
				SFG_TRACE("[RUN] {0}::{1}", test.suite, test.name);

				const bool ok = test.function != nullptr && test.function();
				if (ok)
				{
					passed++;
					SFG_TRACE("[PASS] {0}::{1}", test.suite, test.name);
				}
				else
				{
					failed++;
					SFG_WARN("[FAIL] {0}::{1}", test.suite, test.name);
				}
			}

			if (failed == 0)
				SFG_TRACE("All tests passed. Passed: {0}, Failed: {1}", passed, failed);
			else
				SFG_ERR("Test run failed. Passed: {0}, Failed: {1}", passed, failed);

			return failed == 0 ? 0 : 1;
		}
	}
}
