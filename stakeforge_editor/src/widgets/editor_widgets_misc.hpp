// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_window_buttons_t
	{
		ui::widget_id_t minimize_frame = NULL_WIDGET;
		ui::widget_id_t maximize_frame = NULL_WIDGET;
		ui::widget_id_t close_frame	   = NULL_WIDGET;
		ui::widget_id_t minimize_icon  = NULL_WIDGET;
		ui::widget_id_t maximize_icon  = NULL_WIDGET;
		ui::widget_id_t close_icon	   = NULL_WIDGET;
	};

	class editor_misc_widgets_t final
	{
	public:
		static editor_window_buttons_t add_window_buttons(
			ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& alternative_frame_color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& icon_color, f32 icon_point_size);
	};
}
