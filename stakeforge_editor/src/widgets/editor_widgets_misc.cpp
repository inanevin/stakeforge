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

			const ui::widget_id_t id = ui.allocate_widget();
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

	ui::widget_id_t editor_misc_widgets_t::add_spacer(ui::ui_context& ui, ui::widget_id_t parent, const vec2f_t& size)
	{
		ui::layout_tree_t& tree = ui.get_tree();

		const ui::widget_id_t id = ui.allocate_widget();
		tree.attach(parent, id);
		ui.set_widget_debug_name(id, "spacer");

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= size;

		return id;
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
