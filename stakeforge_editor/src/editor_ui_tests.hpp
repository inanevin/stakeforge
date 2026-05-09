// Copyright (c) 2025 Inan Evin
#pragma once

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_ui_tests_t
	{
	public:
		static void make_test_general(ui::ui_context& ui);
		static void make_test_text(ui::ui_context& ui);
	};
}
