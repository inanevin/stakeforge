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
#include "ui/widgets/editor_widgets_frames.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widgets_frames_t::make_frame_modal(ui::ui_context& ui, ui::widget_id_t id)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame_light;
		rect.fill_color_b		 = theme.color_frame;
		rect.gradient			 = ui::vg_gradient_e::vertical;
		rect.rounding			 = 4.0f;
		rect.rounding_segs		 = 8;
		rect.aa_thickness		 = theme.aa_thickness;
		ui.get_paint().set_rect(id, rect);
	}
}
