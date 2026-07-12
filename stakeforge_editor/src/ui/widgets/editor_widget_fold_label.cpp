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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/widgets/editor_widget_fold_label.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_fold_label_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_fold_label_config_t& config)
	{
		_ui		= &ui;
		_folded = config.folded;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "fold_label");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value.x		 = 0.0f;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = 1.0f;
		root_in.flow			 = ui::flow_e::column;

		_header = ui.allocate_widget();
		ui.set_widget_debug_name(_header, "fold_label_header");
		tree.attach(_root, _header);

		ui::layout_in_t& header_in = tree.in(_header);
		header_in.flags			   = ui::wf_visible | ui::wf_input;
		header_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		header_in.pos_mode_y	   = ui::pos_mode_e::flow;
		header_in.pos_value.x	   = 0.0f;
		header_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		header_in.size_mode_y	   = ui::axis_mode_e::fixed;
		header_in.size_value	   = {1.0f, theme.item_area_height};
		header_in.flow			   = ui::flow_e::row;
		header_in.child_spacing	   = theme.item_spacing * 0.5f;
		header_in.child_margins	   = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal + config.indentation};

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_header_click;
		ui.get_input().set_listener(_header, listener);

		if (config.sub_item)
			editor_icon_widgets_t::add_sub_item_icon(ui, _header);

		_icon					 = editor_icon_widgets_t::add_icon(ui, _header, ICON_DD_DOWN, theme.icon_default_px_size, theme.color_text0);
		ui::layout_in_t& icon_in = tree.in(_icon);
		icon_in.pos_mode_x		 = ui::pos_mode_e::flow;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value		 = {0.0f, 0.5f};
		icon_in.anchor_x		 = ui::anchor_e::start;
		icon_in.anchor_y		 = ui::anchor_e::center;
		tree.draw_order(_icon)	 = tree.draw_order_const(_icon) + 1;

		const ui::widget_id_t label = ui.allocate_widget();
		ui.set_widget_debug_name(label, "fold_label_text");
		tree.attach(_header, label);
		tree.draw_order(label) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& label_in = tree.in(label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(label, config.label != nullptr ? config.label : "");
		paint.set_text(
			label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		if (config.button_style != editor_widget_fold_label_button_style_e::none)
		{
			const ui::widget_id_t filler = ui.allocate_widget();
			ui.set_widget_debug_name(filler, "fold_label_controls_filler");
			tree.attach(_header, filler);

			ui::layout_in_t& filler_in = tree.in(filler);
			filler_in.size_mode_x	   = ui::axis_mode_e::fill;
			filler_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			filler_in.size_value	   = {1.0f, 1.0f};

			if (config.button_style == editor_widget_fold_label_button_style_e::container_buttons)
			{
				_add_button	  = editor_icon_widgets_t::add_naked_icon_button(ui, _header, ICON_PLUS, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
				_reset_button = editor_icon_widgets_t::add_naked_icon_button(ui, _header, ICON_RESET, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);

				tree.draw_order(_add_button)						  = tree.draw_order_const(_header) + 1;
				tree.draw_order(tree.node(_add_button).first_child)	  = tree.draw_order_const(_add_button);
				tree.draw_order(_reset_button)						  = tree.draw_order_const(_header) + 1;
				tree.draw_order(tree.node(_reset_button).first_child) = tree.draw_order_const(_reset_button);
			}
			else if (config.button_style == editor_widget_fold_label_button_style_e::container_item_buttons)
			{
				_remove_button = editor_icon_widgets_t::add_naked_icon_button(ui, _header, ICON_CROSS, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);

				tree.draw_order(_remove_button)						   = tree.draw_order_const(_header) + 1;
				tree.draw_order(tree.node(_remove_button).first_child) = tree.draw_order_const(_remove_button);
			}
		}

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "fold_label_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_value.x	 = 1.0f;
		body_in.flow			 = ui::flow_e::column;

		refresh();
	}

	void editor_widget_fold_label_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui			   = nullptr;
		_root		   = NULL_WIDGET;
		_header		   = NULL_WIDGET;
		_icon		   = NULL_WIDGET;
		_body		   = NULL_WIDGET;
		_add_button	   = NULL_WIDGET;
		_reset_button  = NULL_WIDGET;
		_remove_button = NULL_WIDGET;
		_folded		   = false;
	}

	void editor_widget_fold_label_t::set_fold(bool folded)
	{
		_folded = folded;
		refresh();
	}

	void editor_widget_fold_label_t::clear_children()
	{
		ui::layout_tree_t& tree	 = _ui->get_tree();
		ui::widget_id_t	   child = tree.node(_body).first_child;
		while (child != NULL_WIDGET)
		{
			const ui::widget_id_t next = tree.node(child).next_sibling;
			_ui->deallocate_widget(child);
			child = next;
		}
		refresh();
	}

	void editor_widget_fold_label_t::refresh()
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

	void editor_widget_fold_label_t::on_header_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_fold_label_t& fold = *static_cast<editor_widget_fold_label_t*>(user_data);
		fold.set_fold(!fold._folded);
	}
}
