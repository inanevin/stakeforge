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
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void style_button(ui::paint_layer_t& paint, ui::widget_id_t id)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui::vg_rect_paint_t	  rect	= {};
			rect.fill_color_a			= theme.color_panel_light1;
			rect.fill_color_b			= theme.color_panel_light;
			rect.outline_color			= theme.color_outline_light;
			rect.outline_thickness		= theme.outline_thickness;
			rect.rounding				= theme.item_rounding;
			rect.rounding_segs			= 4;
			paint.set_rect(id, rect);
			paint.set_hover_color(id, theme.color_panel_light2);
			paint.set_press_color(id, theme.color_panel);
			paint.set_focus_color(id, theme.color_accent0);
		}

		void set_label_text(ui::ui_context& ui, ui::widget_id_t id, const char* text, const vec4f_t& color)
		{
			const editor_theme_t& theme = editor_theme_t::get();

			ui.set_widget_text(id, text != nullptr ? text : "");
			ui.get_paint().set_text(id,
									ui.widget_text(id),
									ui.widget_text_len(id),
									{.font = theme.font_default, .color = color.w == 0.0f ? theme.color_text0 : color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
	}

	void editor_widget_button_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_button_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "button");
		tree.attach(parent, _root);

		if (config.elevate_draw_order)
			tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);

		root_in.flags = ui::wf_visible | ui::wf_input;

		apply_editor_widget_width(root_in, config.width);

		root_in.size_mode_y	 = ui::axis_mode_e::fixed;
		root_in.size_value.y = theme.item_height;

		style_button(paint, _root);
		paint.set_disabled_color(_root, theme.color_frame_dark);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "button_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& label_in = tree.in(_label);

		label_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value	= {0.5f, 0.5f};
		label_in.anchor_x	= ui::anchor_e::center;
		label_in.anchor_y	= ui::anchor_e::center;

		set_label_text(ui, _label, config.text, theme.color_text0);
		paint.set_disabled_color(_label, theme.color_text_disabled);
		paint.set_state_source(_label, _root);
	}

	void editor_widget_button_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_label	= NULL_WIDGET;
		_config = {};
	}

	void editor_widget_button_t::set_text(const char* text, const vec4f_t& color)
	{
		_config.text = text;

		set_label_text(*_ui, _label, text, color);
	}
}
