/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "ui/widgets/editor_widgets_draws.hpp"
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <sfg/math/vec2f.hpp>

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
