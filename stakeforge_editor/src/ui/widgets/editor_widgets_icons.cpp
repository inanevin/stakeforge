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
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
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

		const ui::widget_id_t id = ui.allocate_widget();
		tree.attach(parent, id);
		tree.draw_order(id) = tree.draw_order_const(parent) + 1;
		ui.set_widget_debug_name(id, "icon");

		ui::layout_in_t& in = tree.in(id);
		in.flags			= ui::wf_visible;
		in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value		= {0.5f, 0.5f};
		in.anchor_x			= ui::anchor_e::center;
		in.anchor_y			= ui::anchor_e::center;

		ui.set_widget_text(id, icon);
		paint.set_text(id, ui.widget_text(id), ui.widget_text_len(id), {.font = theme.font_icons, .color = color, .point_size = point_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		return id;
	}

	ui::widget_id_t editor_icon_widgets_t::add_naked_icon_button(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 size, const vec4f_t& color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& disabled_color)
	{
		ui::layout_tree_t& tree	 = ui.get_tree();
		ui::paint_layer_t& paint = ui.get_paint();

		const ui::widget_id_t wrapper = ui.allocate_widget();
		tree.attach(parent, wrapper);
		tree.draw_order(wrapper) = tree.draw_order_const(parent) + 1;
		ui.set_widget_debug_name(wrapper, "icon_button_wrapper");

		ui::layout_in_t& in = tree.in(wrapper);
		in.flags			= ui::wf_visible | ui::wf_input;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value.y		= 0.5f;
		in.anchor_y			= ui::anchor_e::center;
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {size, size};

		const ui::widget_id_t icon_widget = add_icon(ui, wrapper, icon, size, color);
		paint.set_hover_color(icon_widget, hover_color);
		paint.set_press_color(icon_widget, press_color);
		paint.set_disabled_color(icon_widget, disabled_color);
		paint.set_state_source(icon_widget, wrapper);

		return wrapper;
	}
}
