/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "widgets/editor_widgets_buttons.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widgets_buttons_t::make_button_modal(ui::ui_context& ui, ui::widget_id_t id)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::paint_layer_t&	  paint = ui.get_paint();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_bg4;
		rect.fill_color_b		 = theme.color_bg4;
		rect.rounding			 = 3.0f;
		rect.rounding_segs		 = 6;
		rect.aa_thickness		 = theme.aa_thickness;
		paint.set_rect(id, rect);
		paint.set_hover_color(id, theme.color_bg3);
		paint.set_press_color(id, theme.color_bg1);
	}
}
