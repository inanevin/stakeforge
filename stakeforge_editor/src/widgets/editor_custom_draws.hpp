// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class vg_canvas_t;
}

namespace sfg
{
	class editor_custom_draws_t final
	{
	public:
		static void add_leaned_convex_rect(ui::vg_canvas_t& canvas, const vec2f_t& pos, const vec2f_t& size, f32 lean, const ui::vg_convex_paint_t& paint, const ui::ui_render_state_t& state, u32 draw_order);
	};
}
