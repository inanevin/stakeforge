// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct vec4f_t;
}

namespace sfg
{
	class editor_dividers_t final
	{
	public:
		static ui::widget_id_t make_divider_horizontal(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color);
		static ui::widget_id_t make_divider_vertical(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color);
		static ui::widget_id_t make_divider_horizontal_dropshadow(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color, bool flip = false);
		static ui::widget_id_t make_divider_vertical_dropshadow(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color, bool flip = false);
	};
}
