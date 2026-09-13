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
#include "editor_widget_fold.hpp"
#include "editor_widgets_icons.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_fold_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_fold_config_t& config)
	{
		_ui				 = &ui;
		_folded			 = config.folded;
		_on_pressed		 = config.on_pressed;
		_on_fold_changed = config.on_fold_changed;
		_user_data		 = config.user_data;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "fold");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value.x		 = 0.0f;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = 1.0f;
		root_in.flow			 = ui::flow_e::column;

		if (config.background_frame)
			paint.set_rect(_root, config.background);

		if (_on_pressed != nullptr)
		{
			root_in.flags |= ui::wf_input;
			ui.get_input().set_listener(_root, {.on_press = on_background_press, .user_data = this});
		}

		_header = ui.allocate_widget();
		ui.set_widget_debug_name(_header, "fold_header");
		tree.attach(_root, _header);

		if (config.background_frame || _on_pressed != nullptr)
			tree.draw_order(_header) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& header_in = tree.in(_header);
		header_in.flags			   = ui::wf_visible | ui::wf_input;
		header_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		header_in.pos_mode_y	   = ui::pos_mode_e::flow;
		header_in.pos_value.x	   = 0.0f;
		header_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		header_in.size_mode_y	   = ui::axis_mode_e::fixed;
		header_in.size_value	   = {1.0f, theme.item_height};
		header_in.flow			   = ui::flow_e::row;
		header_in.child_spacing	   = theme.item_spacing * 0.5f;
		header_in.child_margins	   = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		if (!config.background_frame && config.header_frame)
		{
			paint.set_rect(_header,
						   {
							   .fill_color_a	  = {theme.color_accent0_dim.x, theme.color_accent0_dim.y, theme.color_accent0_dim.z, 0.8f},
							   .fill_color_b	  = {theme.color_accent0_dim.x, theme.color_accent0_dim.y, theme.color_accent0_dim.z, 0.5f},
							   .outline_color	  = theme.color_outline,
							   .outline_thickness = theme.outline_thickness,
							   .gradient		  = ui::vg_gradient_e::horizontal,
						   });
			paint.set_hover_color(_header, theme.color_accent0);
			paint.set_press_color(_header, theme.color_panel);
		}

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_header_click;
		listener.on_press			   = on_background_press;
		ui.get_input().set_listener(_header, listener);

		const ui::widget_id_t icon_frame = ui.allocate_widget();
		ui.set_widget_debug_name(icon_frame, "fold_icon_frame");
		tree.attach(_header, icon_frame);

		ui::layout_in_t& icon_frame_in = tree.in(icon_frame);
		icon_frame_in.size_mode_x	   = ui::axis_mode_e::fixed;
		icon_frame_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		icon_frame_in.size_value	   = {theme.item_height, 1.0f};

		_icon				   = editor_icon_widgets_t::add_icon(ui, icon_frame, ICON_DD_DOWN, theme.icon_default_px_size, theme.color_text0);
		tree.draw_order(_icon) = tree.draw_order_const(_icon) + 1;

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "fold_label");
		tree.attach(_header, _label);
		tree.draw_order(_label) = tree.draw_order_const(_header) + 1;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		set_text(config.label != nullptr ? config.label : "");

		if (config.settings_button)
		{
			_settings_button					= editor_icon_widgets_t::add_naked_icon_button(ui, _header, ICON_SETTINGS, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
			tree.draw_order(_settings_button)	= tree.draw_order_const(_header) + 1;
			const ui::widget_id_t settings_icon = tree.node(_settings_button).first_child;

			tree.draw_order(settings_icon) = tree.draw_order_const(_settings_button);

			ui::layout_in_t& settings_in = tree.in(_settings_button);
			settings_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
			settings_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
			settings_in.pos_value		 = {1.0f, 0.5f};
			settings_in.anchor_x		 = ui::anchor_e::end;
			settings_in.anchor_y		 = ui::anchor_e::center;
		}

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "fold_body");
		tree.attach(_root, _body);
		tree.draw_order(_body) = tree.draw_order_const(_header);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_value.x	 = 1.0f;
		body_in.flow			 = ui::flow_e::column;
		body_in.child_spacing	 = theme.item_spacing;

		refresh();
	}

	void editor_widget_fold_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui				 = nullptr;
		_on_pressed		 = nullptr;
		_on_fold_changed = nullptr;
		_user_data		 = nullptr;
		_root			 = NULL_WIDGET;
		_header			 = NULL_WIDGET;
		_icon			 = NULL_WIDGET;
		_label			 = NULL_WIDGET;
		_body			 = NULL_WIDGET;
		_settings_button = NULL_WIDGET;
		_folded			 = false;
	}

	void editor_widget_fold_t::set_fold(bool folded)
	{
		_folded = folded;
		refresh();

		if (_on_fold_changed != nullptr)
			_on_fold_changed(_folded, _user_data);
	}

	void editor_widget_fold_t::refresh()
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.flags			 = _folded ? 0 : ui::wf_visible;
		body_in.size_mode_y		 = _folded ? ui::axis_mode_e::fixed : ui::axis_mode_e::sum_children;
		body_in.size_value.y	 = _folded ? 0.0f : 1.0f;

		_ui->set_widget_text(_icon, _folded ? ICON_DD_RIGHT : ICON_DD_DOWN);
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();
		paint.set_text(
			_icon, _ui->widget_text(_icon), _ui->widget_text_len(_icon), {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_widget_fold_t::set_text(const char* text)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->set_widget_text(_label, text);
		_ui->get_paint().set_text(_label,
								  _ui->widget_text(_label),
								  _ui->widget_text_len(_label),
								  {
									  .font		   = theme.font_default,
									  .color	   = theme.color_text0,
									  .point_size  = theme.text_default_px_size,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_widget_fold_t::on_background_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_fold_t& fold = *static_cast<editor_widget_fold_t*>(user_data);

		if (fold._on_pressed != nullptr)
			fold._on_pressed(fold._user_data);
	}

	void editor_widget_fold_t::on_header_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_fold_t& fold = *static_cast<editor_widget_fold_t*>(user_data);
		fold.set_fold(!fold._folded);
	}
}
