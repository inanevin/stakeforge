// Copyright (c) 2025 Inan Evin

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
