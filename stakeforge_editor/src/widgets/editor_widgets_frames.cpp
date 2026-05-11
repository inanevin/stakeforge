// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_frames.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widgets_frames_t::make_frame_modal(ui::ui_context& ui, ui::widget_id_t id)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_bg3;
		rect.fill_color_b		 = theme.color_bg2;
		rect.gradient			 = ui::vg_gradient_e::vertical;
		rect.rounding			 = 4.0f;
		rect.rounding_segs		 = 8;
		rect.aa_thickness		 = theme.aa_thickness;
		ui.get_paint().set_rect(id, rect);
	}
}
