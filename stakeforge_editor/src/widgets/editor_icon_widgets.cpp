// Copyright (c) 2025 Inan Evin

#include "widgets/editor_icon_widgets.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	ui::widget_id_t editor_icon_widgets_t::add_icon(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 point_size, const vec4f_t& color)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();

		const ui::widget_id_t id = tree.allocate();
		tree.attach(parent, id);
		tree.draw_order(id) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& in = tree.in(id);
		in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value		= {0.5f, 0.5f};
		in.anchor_x			= ui::anchor_e::center;
		in.anchor_y			= ui::anchor_e::center;

		ui.set_widget_text(id, icon);
		paint.set_text(id, ui.widget_text(id), ui.widget_text_len(id), {.font = theme.font_icons, .color = color, .point_size = point_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::grayscale});

		return id;
	}
}
