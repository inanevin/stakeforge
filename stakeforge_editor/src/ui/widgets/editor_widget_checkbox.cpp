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
#include "ui/widgets/editor_widget_checkbox.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_checkbox_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_checkbox_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "checkbox");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {theme.item_height, theme.item_height};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.outline_color		 = theme.color_outline_light;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);
		paint.set_focus_color(_root, theme.color_accent0);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		listener.on_key				   = on_key;
		ui.get_input().set_listener(_root, listener);

		_check = ui.allocate_widget();
		ui.set_widget_debug_name(_check, "checkbox_check");
		tree.attach(_root, _check);
		tree.draw_order(_check) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& check_in = tree.in(_check);
		check_in.flags			  = 0;
		check_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		check_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		check_in.pos_value		  = {0.5f, 0.5f};
		check_in.anchor_x		  = ui::anchor_e::center;
		check_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(_check, ICON_CHECK);
		paint.set_text(
			_check, ui.widget_text(_check), ui.widget_text_len(_check), {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		update_field_data(config.field);
	}

	void editor_checkbox_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui	   = nullptr;
		_root  = NULL_WIDGET;
		_check = NULL_WIDGET;
		_fields.resize(0);
		_config	 = {};
		_checked = false;
		_mixed	 = false;
	}

	void editor_checkbox_t::update_field_data(editor_checkbox_field_t field)
	{
		SFG_ASSERT(field.fields.size > 0);
		SFG_ASSERT(field.fields.data != nullptr);
		for (size_t i = 0; i < field.fields.size; ++i)
			SFG_ASSERT(field.fields.data[i] != nullptr);

		if (field.fields.data != _fields.data())
			_fields.assign(field.fields.data, field.fields.data + field.fields.size);
		field.fields  = {.data = _fields.data(), .size = _fields.size()};
		_config.field = field;
		_checked	  = *field.fields.data[0] != 0;
		_mixed		  = false;
		for (size_t i = 1; i < field.fields.size; ++i)
		{
			if ((*field.fields.data[i] != 0) != _checked)
			{
				_mixed = true;
				break;
			}
		}
		refresh();
	}

	void editor_checkbox_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_checkbox_t::refresh()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		_ui->set_widget_text(_check, _mixed ? ICON_CROSS : ICON_CHECK);
		_ui->get_paint().set_text(_check,
								  _ui->widget_text(_check),
								  _ui->widget_text_len(_check),
								  {.font = theme.font_icons, .color = _mixed ? theme.color_accent_warn : theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		_ui->get_tree().in(_check).flags = (_checked || _mixed) ? ui::wf_visible : 0;
	}

	void editor_checkbox_t::toggle()
	{
		if (_config.callbacks.edit_begin != nullptr)
			_config.callbacks.edit_begin(_config.callbacks.user_data);
		_checked = !_checked;
		_mixed	 = false;
		modify_field();
		refresh();
		if (_config.callbacks.edit_submitted != nullptr)
			_config.callbacks.edit_submitted(_config.callbacks.user_data);
	}

	void editor_checkbox_t::modify_field()
	{
		SFG_ASSERT(_config.field.fields.size > 0);
		SFG_ASSERT(_config.field.fields.data != nullptr);
		for (size_t i = 0; i < _config.field.fields.size; ++i)
			*_config.field.fields.data[i] = _checked ? 1 : 0;
		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	void editor_checkbox_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_checkbox_t*>(user_data)->toggle();
	}

	void editor_checkbox_t::on_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press || ev.key != static_cast<u16>(input_code::key_return))
			return;

		static_cast<editor_checkbox_t*>(user_data)->toggle();
	}
}
