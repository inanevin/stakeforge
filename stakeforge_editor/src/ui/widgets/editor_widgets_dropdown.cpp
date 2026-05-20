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
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_dropdown_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_dropdown_config_t& config)
	{
		SFG_ASSERT(config.items != nullptr);

		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "dropdown");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::sum_children;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.pos_mode_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::pos_mode_e::relative_in_parent : ui::pos_mode_e::flow;
		root_in.pos_value.y		 = config.pos_y == editor_dropdown_pos_y_e::center ? 0.5f : 0.0f;
		root_in.anchor_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::anchor_e::center : ui::anchor_e::start;
		root_in.flow			 = config.width == editor_dropdown_width_e::sum_children ? ui::flow_e::row : ui::flow_e::none;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		if (config.width == editor_dropdown_width_e::parent_relative)
			root_in.size_mode_x = ui::axis_mode_e::parent_relative;
		else if (config.width == editor_dropdown_width_e::fixed)
		{
			root_in.size_mode_x	 = ui::axis_mode_e::fixed;
			root_in.size_value.x = config.fixed_width;
		}

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.rounding			 = theme.item_rounding;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.outline_color		 = theme.color_panel_light;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);

		ui::listener_bundle_t root_listener = {};
		root_listener.user_data				= this;
		root_listener.on_click				= on_root_click;
		ui.get_input().set_listener(_root, root_listener);

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "dropdown_title");
		tree.attach(_root, _title);
		tree.draw_order(_title) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& title_in = tree.in(_title);
		title_in.flags			  = ui::wf_visible;
		title_in.size_mode_x	  = ui::axis_mode_e::fixed;
		title_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		title_in.size_value		  = {0.0f, 1.0f};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			title_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			title_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value	= {0.0f, 0.5f};
			title_in.anchor_y	= ui::anchor_e::center;
		}
		else
		{
			title_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value.y = 0.5f;
			title_in.anchor_y	 = ui::anchor_e::center;
		}

		paint.set_text(_title, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_icon_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_icon_frame, "dropdown_icon");
		tree.attach(_root, _icon_frame);
		tree.draw_order(_icon_frame) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& icon_in = tree.in(_icon_frame);
		icon_in.flags			 = ui::wf_visible;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			icon_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value  = {1.0f, 0.5f};
			icon_in.anchor_x   = ui::anchor_e::end;
			icon_in.anchor_y   = ui::anchor_e::center;
		}
		else
		{
			icon_in.pos_mode_y	= ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value.y = 0.5f;
			icon_in.anchor_y	= ui::anchor_e::center;
		}
		editor_icon_widgets_t::add_icon(ui, _icon_frame, ICON_DD_DOWN, theme.icon_default_px_size, theme.color_text1);

		refresh_title();
	}

	void editor_dropdown_t::uninit()
	{
		close();
		_ui->deallocate_widget(_root);

		_ui			= nullptr;
		_root		= NULL_WIDGET;
		_title		= NULL_WIDGET;
		_icon_frame = NULL_WIDGET;
		_config		= {};
	}

	void editor_dropdown_t::close()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		if (popup != nullptr)
			popup->close_popup();
	}

	void editor_dropdown_t::refresh_title()
	{
		SFG_ASSERT(_ui != nullptr);
		_ui->set_widget_text(_title, get_selected_text());
		ui::layout_in_t& title_in = _ui->get_tree().in(_title);
		title_in.size_value.x	  = static_cast<f32>(_ui->widget_text_len(_title)) * editor_theme_t::get().text_default_px_size * 0.7f;
		_ui->get_paint().set_text(
			_title,
			_ui->widget_text(_title),
			_ui->widget_text_len(_title),
			{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text0, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	u16 editor_dropdown_t::get_selected() const
	{
		return _config.selected != nullptr ? _config.selected(_config.user_data) : (_config.item_count > 0 ? _config.items[0].value : 0);
	}

	const char* editor_dropdown_t::get_selected_text() const
	{
		if (!_config.title_from_selection && _config.title != nullptr)
			return _config.title;

		const u16 selected = get_selected();
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			if (_config.items[i].value == selected)
				return _config.items[i].text;
		}
		return _config.item_count > 0 ? _config.items[0].text : "";
	}

	void editor_dropdown_t::on_root_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_dropdown_t&		   dropdown = *static_cast<editor_dropdown_t*>(user_data);
		editor_popup_controller_t* popup	= editor_popup_controller_t::find(*dropdown._ui);
		SFG_ASSERT(popup != nullptr);

		editor_popup_item_desc_t items[16] = {};
		SFG_ASSERT(dropdown._config.item_count <= 16);
		const u16 selected = dropdown.get_selected();
		for (u32 i = 0; i < dropdown._config.item_count; ++i)
		{
			items[i].text	  = dropdown._config.items[i].text;
			items[i].id		  = dropdown._config.items[i].value;
			items[i].selected = dropdown._config.items[i].value == selected;
		}

		const editor_theme_t&	theme	 = editor_theme_t::get();
		const ui::layout_out_t& root_out = dropdown._ui->get_tree().out(dropdown._root);
		editor_popup_desc_t		desc	 = {};
		desc.items						 = items;
		desc.item_count					 = dropdown._config.item_count;
		desc.pos						 = {root_out.pos.x, root_out.pos.y + root_out.size.y + theme.item_spacing};
		desc.width						 = root_out.size.x;
		desc.pressed					 = on_popup_item_pressed;
		desc.user_data					 = &dropdown;
		popup->request_popup(desc);
	}

	void editor_dropdown_t::on_popup_item_pressed(u16 value, void* user_data)
	{
		editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(user_data);
		if (dropdown._config.pressed != nullptr)
			dropdown._config.pressed(value, dropdown._config.user_data);
		dropdown.refresh_title();
	}
}
