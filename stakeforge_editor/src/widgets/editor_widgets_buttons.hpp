// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_widgets_buttons_t final
	{
	public:
		static void make_button_modal(ui::ui_context& ui, ui::widget_id_t id);
	};
}
