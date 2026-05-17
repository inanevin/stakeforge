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
#include "ui/widgets/editor_widgets_buttons.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widgets_buttons_t::make_button_modal(ui::ui_context& ui, ui::widget_id_t frame, ui::widget_id_t label)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();

		ui::layout_in_t& frame_in = tree.in(frame);
		frame_in.flags			  = ui::wf_overlay;
		frame_in.size_mode_x	  = ui::axis_mode_e::fill;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value		  = {1.0f, 1.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_light;
		rect.fill_color_b		 = theme.color_light;
		rect.rounding			 = 3.0f;
		rect.rounding_segs		 = 6;
		rect.aa_thickness		 = theme.aa_thickness;
		paint.set_rect(frame, rect);
		paint.set_hover_color(frame, theme.color_panel_light);
		paint.set_press_color(frame, theme.color_frame_light);

		ui::layout_in_t& label_in = tree.in(label);
		label_in.flags			  = ui::wf_overlay;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;
		paint.set_text(label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_small_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}
}
