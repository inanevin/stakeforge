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
#include "ui/widgets/editor_widgets_file_menu.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void set_rect_color(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& color)
		{
			ui::paint_def_t& def  = paint.def(id);
			def.rect.fill_color_a = color;
			def.rect.fill_color_b = color;
			def.rect.gradient	  = ui::vg_gradient_e::none;
		}
	}

	void editor_file_menu_t::handle_top_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_top_click(id, btn);
	}

	void editor_file_menu_t::handle_top_hover(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_top_hover(id);
	}

	void editor_file_menu_t::handle_action_menu_closed(void* user_data)
	{
		editor_file_menu_t& menu = *static_cast<editor_file_menu_t*>(user_data);
		menu._open				 = false;
		menu.refresh_top_frames();
	}

	void editor_file_menu_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_file_menu_item_desc_t* items, u16 item_count, const editor_file_menu_style_t& style, void (*command_fn)(u16 command, void* user_data), void* command_user_data)
	{
		SFG_ASSERT(item_count <= MAX_TOP_ITEMS);

		_ui				   = &ui;
		_items			   = items;
		_item_count		   = item_count;
		_style			   = style;
		_command_fn		   = command_fn;
		_command_user_data = command_user_data;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "file_menu");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::sum_children;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {0.0f, 1.0f};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		ui::listener_bundle_t top_listener = {};
		top_listener.user_data			   = this;
		top_listener.on_click			   = handle_top_click;
		top_listener.on_hover_enter		   = handle_top_hover;

		for (u32 i = 0; i < item_count; ++i)
		{
			_top_frames[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_top_frames[i], "file_menu_top_item");
			tree.attach(_root, _top_frames[i]);
			tree.draw_order(_top_frames[i]) = tree.draw_order_const(_root) + 1;

			ui::layout_in_t& in = tree.in(_top_frames[i]);
			in.flags			= ui::wf_visible | ui::wf_input;
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {style.button_width, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = style.frame_color;
			rect.fill_color_b		 = style.frame_color;
			paint.set_rect(_top_frames[i], rect);
			paint.set_hover_color(_top_frames[i], style.hover_color);
			paint.set_press_color(_top_frames[i], style.press_color);
			ui.get_input().set_listener(_top_frames[i], top_listener);

			_top_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_top_labels[i], "file_menu_top_label");
			tree.attach(_top_frames[i], _top_labels[i]);
			tree.draw_order(_top_labels[i]) = tree.draw_order_const(_top_frames[i]) + 1;

			ui::layout_in_t& label_in = tree.in(_top_labels[i]);
			label_in.flags			  = ui::wf_visible;
			label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value		  = {0.5f, 0.5f};
			label_in.anchor_x		  = ui::anchor_e::center;
			label_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(_top_labels[i], items[i].text);
			paint.set_text(_top_labels[i],
						   ui.widget_text(_top_labels[i]),
						   ui.widget_text_len(_top_labels[i]),
						   {.font = theme.font_default, .color = style.text_color, .point_size = style.text_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
	}

	void editor_file_menu_t::uninit()
	{
		close();
		_ui->deallocate_widget(_root);

		_ui				   = nullptr;
		_items			   = nullptr;
		_item_count		   = 0;
		_style			   = {};
		_command_fn		   = nullptr;
		_command_user_data = nullptr;
		_root			   = NULL_WIDGET;
		_selected_top	   = 0;
		_open			   = false;
		for (u32 i = 0; i < MAX_TOP_ITEMS; ++i)
		{
			_top_frames[i] = NULL_WIDGET;
			_top_labels[i] = NULL_WIDGET;
		}
	}

	void editor_file_menu_t::close()
	{
		if (!_open)
			return;

		editor_action_menu_controller_t* controller = editor_action_menu_controller_t::find(*_ui);
		if (controller != nullptr)
			controller->close_action_menu();
	}

	void editor_file_menu_t::on_top_click(ui::widget_id_t id, ui::mouse_button_e btn)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		open_top(find_top_index(id));
	}

	void editor_file_menu_t::on_top_hover(ui::widget_id_t id)
	{
		if (_open)
			open_top(find_top_index(id));
	}

	void editor_file_menu_t::open_top(u32 index)
	{
		SFG_ASSERT(index < _item_count);

		_selected_top = index;

		editor_action_menu_controller_t* controller = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(controller != nullptr);

		const ui::layout_out_t& out = _ui->get_tree().out(_top_frames[index]);

		editor_action_menu_desc_t desc = {};
		desc.rows					   = _items[index].rows;
		desc.row_count				   = _items[index].row_count;
		desc.pos					   = {out.pos.x, out.pos.y + out.size.y};
		desc.style					   = get_action_menu_style();
		desc.command_fn				   = _command_fn;
		desc.command_user_data		   = _command_user_data;
		desc.closed_fn				   = handle_action_menu_closed;
		desc.closed_user_data		   = this;
		desc.owner_root				   = _root;
		controller->request_action_menu(desc);
		_open = true;
		refresh_top_frames();
	}

	void editor_file_menu_t::refresh_top_frames()
	{
		ui::paint_layer_t& paint = _ui->get_paint();
		for (u32 i = 0; i < _item_count; ++i)
		{
			const vec4f_t color = (_open && i == _selected_top) ? _style.selected_color : _style.frame_color;
			set_rect_color(paint, _top_frames[i], color);
		}
	}

	u32 editor_file_menu_t::find_top_index(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _item_count; ++i)
		{
			if (_top_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	editor_action_menu_style_t editor_file_menu_t::get_action_menu_style() const
	{
		editor_action_menu_style_t style = {};
		style.dropdown_color			 = _style.dropdown_color;
		style.hover_color				 = _style.hover_color;
		style.press_color				 = _style.press_color;
		style.text_color				 = _style.text_color;
		style.shortcut_color			 = _style.shortcut_color;
		style.disabled_text_color		 = editor_theme_t::get().color_text_disabled;
		style.title_color				 = _style.title_color;
		style.title_line_color			 = _style.title_line_color;
		style.icon_color				 = _style.icon_color;
		style.min_width					 = _style.button_width;
		style.row_height				 = _style.row_height;
		style.text_size					 = _style.text_size;
		style.shortcut_size				 = _style.shortcut_size;
		style.title_size				 = _style.title_size;
		style.title_line_thickness		 = _style.title_line_thickness;
		style.icon_size					 = _style.icon_size;
		style.padding_x					 = _style.padding_x;
		style.padding_y					 = _style.padding_y;
		style.shortcut_gap				 = _style.shortcut_gap;
		style.title_gap					 = _style.title_gap;
		return style;
	}
}
