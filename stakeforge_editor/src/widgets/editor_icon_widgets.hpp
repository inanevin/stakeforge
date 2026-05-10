// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_icon_widgets_t final
	{
	public:
		static ui::widget_id_t add_icon(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 point_size, const vec4f_t& color);
	};
}
