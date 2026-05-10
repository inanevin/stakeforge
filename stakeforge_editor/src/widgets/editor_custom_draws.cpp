// Copyright (c) 2025 Inan Evin

#include "widgets/editor_custom_draws.hpp"
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	void editor_custom_draws_t::add_leaned_convex_rect(ui::vg_canvas_t& canvas, const vec2f_t& pos, const vec2f_t& size, f32 lean, const ui::vg_convex_paint_t& paint, const ui::ui_render_state_t& state, u32 draw_order)
	{
		const f32 skew = size.y * lean;

		const vec2f_t p0	  = {pos.x + skew, pos.y};
		const vec2f_t p1	  = {p0.x + size.x, pos.y};
		const vec2f_t p2	  = {pos.x + size.x, pos.y + size.y};
		const vec2f_t p3	  = {pos.x, pos.y + size.y};
		vec2f_t		  path[4] = {p0, p1, p2, p3};

		canvas.add_convex({path, 4}, paint, state, draw_order);
	}
}
