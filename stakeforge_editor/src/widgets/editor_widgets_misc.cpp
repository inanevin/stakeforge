// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_misc.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		ui::widget_id_t add_window_button_frame(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& hover_color, const vec4f_t& press_color)
		{
			ui::layout_tree_t&	   tree			 = ui.get_tree();
			ui::paint_layer_t&	   paint		 = ui.get_paint();
			const ui::layout_in_t& parent_in	 = tree.in_const(parent);
			const bool			   parent_row	 = parent_in.flow == ui::flow_e::row;
			const bool			   parent_column = parent_in.flow == ui::flow_e::column;
			SFG_ASSERT(parent_row || parent_column);

			const ui::widget_id_t id = tree.allocate();
			tree.attach(parent, id);
			tree.draw_order(id) = tree.draw_order_const(parent) + 1;

			ui::layout_in_t& in = tree.in(id);
			in.flags			= ui::wf_visible | ui::wf_input;
			in.size_mode_x		= parent_row ? ui::axis_mode_e::fill : ui::axis_mode_e::parent_relative;
			in.size_mode_y		= parent_row ? ui::axis_mode_e::parent_relative : ui::axis_mode_e::fill;
			in.size_value		= {1.0f, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = frame_color;
			rect.fill_color_b		 = frame_color;
			paint.set_rect(id, rect);
			paint.set_hover_color(id, hover_color);
			paint.set_press_color(id, press_color);

			return id;
		}
	}

	editor_window_buttons_t editor_misc_widgets_t::add_window_buttons(
		ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& alternative_frame_color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& icon_color, f32 icon_point_size)
	{
		editor_window_buttons_t buttons = {};
		buttons.minimize_frame			= add_window_button_frame(ui, parent, frame_color, hover_color, press_color);
		buttons.minimize_icon			= editor_icon_widgets_t::add_icon(ui, buttons.minimize_frame, ICON_WINDOW_MINIMIZE, icon_point_size, icon_color);
		buttons.maximize_frame			= add_window_button_frame(ui, parent, frame_color, hover_color, press_color);
		buttons.maximize_icon			= editor_icon_widgets_t::add_icon(ui, buttons.maximize_frame, ICON_WINDOW_MAXIMIZE, icon_point_size, icon_color);
		buttons.close_frame				= add_window_button_frame(ui, parent, alternative_frame_color, hover_color, press_color);
		buttons.close_icon				= editor_icon_widgets_t::add_icon(ui, buttons.close_frame, ICON_WINDOW_CLOSE, icon_point_size, icon_color);
		return buttons;
	}
}
