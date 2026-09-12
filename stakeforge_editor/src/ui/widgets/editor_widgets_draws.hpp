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
	class vec2f;

	class editor_custom_draws_t final
	{
	public:
		static void add_leaned_convex_rect(ui::vg_canvas_t& canvas, const vec2f_t& pos, const vec2f_t& size, f32 lean, const ui::vg_convex_paint_t& paint, const ui::ui_render_state_t& state, u32 draw_order);
	};
}
