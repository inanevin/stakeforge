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
		static ui::widget_id_t add_divider_hor(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient);
		static ui::widget_id_t add_divider_ver(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient);
	};
}
