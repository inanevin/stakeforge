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
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

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
		ui.set_widget_debug_name(id, "icon");

		ui::layout_in_t& in = tree.in(id);
		in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value		= {0.5f, 0.5f};
		in.anchor_x			= ui::anchor_e::center;
		in.anchor_y			= ui::anchor_e::center;

		ui.set_widget_text(id, icon);
		paint.set_text(id, ui.widget_text(id), ui.widget_text_len(id), {.font = theme.font_icons, .color = color, .point_size = point_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		return id;
	}

	ui::widget_id_t editor_icon_widgets_t::add_sub_item_icon(ui::ui_context& ui, ui::widget_id_t parent)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();

		const ui::widget_id_t frame = ui.allocate_widget();
		ui.set_widget_debug_name(frame, "sub_item_icon_frame");
		tree.attach(parent, frame);

		ui::layout_in_t& frame_in = tree.in(frame);
		frame_in.size_mode_x	  = ui::axis_mode_e::fixed;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value		  = {theme.item_height, 1.0f};

		editor_icon_widgets_t::add_icon(ui, frame, ICON_L, theme.item_height * 0.65f, theme.color_frame);
		return frame;
	}

	ui::widget_id_t editor_icon_widgets_t::add_naked_icon_button(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 size, const vec4f_t& color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& disabled_color)
	{
		ui::layout_tree_t& tree	 = ui.get_tree();
		ui::paint_layer_t& paint = ui.get_paint();

		const ui::widget_id_t wrapper = ui.allocate_widget();
		tree.attach(parent, wrapper);
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
