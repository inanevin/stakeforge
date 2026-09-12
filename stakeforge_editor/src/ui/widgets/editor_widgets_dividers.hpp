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
		static ui::widget_id_t add_divider_ver(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, f32 height, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient);
	};
}
