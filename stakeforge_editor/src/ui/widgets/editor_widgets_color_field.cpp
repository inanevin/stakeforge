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
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_color_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "color_field");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable;
		apply_editor_widget_width(root_in, config.width);
		root_in.size_mode_y	  = ui::axis_mode_e::fixed;
		root_in.size_value.y  = theme.item_height;
		root_in.child_margins = {theme.outline_thickness * 2.0f, 0.0f, theme.outline_thickness * 2.0f, 0.0f};

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		ui.get_input().set_listener(_root, listener);

		_swatch = ui.allocate_widget();
		ui.set_widget_debug_name(_swatch, "color_field_swatch");
		tree.attach(_root, _swatch);

		ui::layout_in_t& swatch_in = tree.in(_swatch);
		swatch_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		swatch_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		swatch_in.size_value	   = {1.0f, 1.0f};
		paint.set_focus_color(_swatch, theme.color_accent0);
		paint.set_state_source(_swatch, _root);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "color_field_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = 0;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;
		ui.set_widget_text(_label, "Mixed");

		update_field_data(config.field);
	}

	void editor_color_field_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_swatch = NULL_WIDGET;
		_label	= NULL_WIDGET;
		_fields.resize(0);
		_config		 = {};
		_color		 = {};
		_mixed		 = false;
		_edit_active = false;
		_edit_dirty	 = false;
	}

	void editor_color_field_t::set_color(const color_t& color)
	{
		_color = color;
		_mixed = false;
		modify_field();
		refresh_color();
	}

	void editor_color_field_t::update_field_data(editor_color_field_field_t field)
	{
		if (field.fields.data != _fields.data())
			_fields.assign(field.fields.data, field.fields.data + field.fields.size);

		field.fields  = {.data = _fields.data(), .size = _fields.size()};
		_config.field = field;
		_color		  = *field.fields.data[0];
		_mixed		  = false;

		for (size_t i = 1; i < field.fields.size; ++i)
		{
			if (*field.fields.data[i] != _color)
			{
				_mixed = true;
				break;
			}
		}
		refresh_color();
	}

	void editor_color_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_color_field_t::refresh_color()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		ui::vg_rect_paint_t	  rect	= {};
		rect.fill_color_a			= _mixed ? theme.color_frame : _color.to_vector();
		rect.fill_color_b			= rect.fill_color_a;
		rect.outline_color			= theme.color_outline_light;
		rect.outline_thickness		= theme.outline_thickness;
		rect.rounding				= theme.item_rounding;
		rect.rounding_segs			= 4;
		rect.aa_thickness			= theme.aa_thickness;
		paint.set_rect(_swatch, rect);
		tree.in(_swatch).flags = ui::wf_visible;
		tree.in(_label).flags  = _mixed ? ui::wf_visible : 0;

		if (_mixed)
		{
			paint.set_text(_label,
						   _ui->widget_text(_label),
						   _ui->widget_text_len(_label),
						   {.font = theme.font_default, .color = theme.color_accent_warn, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
		else
		{
			paint.clear(_label);
		}
	}

	void editor_color_field_t::modify_field()
	{
		for (size_t i = 0; i < _config.field.fields.size; ++i)
			*_config.field.fields.data[i] = _color;

		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	void editor_color_field_t::begin_edit()
	{
		if (_edit_active)
			return;
		_edit_active = true;
		if (_config.callbacks.edit_begin != nullptr)
			_config.callbacks.edit_begin(_config.callbacks.user_data);
	}

	void editor_color_field_t::submit_edit()
	{
		if (!_edit_dirty)
		{
			_edit_active = false;
			return;
		}
		if (_config.callbacks.edit_submitted != nullptr)
			_config.callbacks.edit_submitted(_config.callbacks.user_data);
		_edit_active = false;
		_edit_dirty	 = false;
	}

	void editor_color_field_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_color_field_t&	   field = *static_cast<editor_color_field_t*>(user_data);
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*field._ui);

		popup->request_color_wheel_popup({
			.fields			 = {.data = field._fields.data(), .size = field._fields.size()},
			.edit_begin		 = on_color_wheel_edit_begin,
			.on_data_changed = on_color_wheel_data_changed,
			.closed			 = on_color_wheel_popup_closed,
			.user_data		 = &field,
			.pos			 = pos,
		});
	}

	void editor_color_field_t::on_color_wheel_edit_begin(void* user_data)
	{
		static_cast<editor_color_field_t*>(user_data)->begin_edit();
	}

	void editor_color_field_t::on_color_wheel_data_changed(void* user_data)
	{
		editor_color_field_t& field	  = *static_cast<editor_color_field_t*>(user_data);
		bool				  changed = field._mixed;
		for (size_t i = 0; i < field._config.field.fields.size; ++i)
			changed |= *field._config.field.fields.data[i] != field._color;
		if (!changed)
			return;

		field.refresh_field_data();
		field._edit_dirty = true;
		if (field._config.callbacks.edited != nullptr)
			field._config.callbacks.edited(field._config.callbacks.user_data);
	}

	void editor_color_field_t::on_color_wheel_popup_closed(void* user_data)
	{
		static_cast<editor_color_field_t*>(user_data)->submit_edit();
	}
}
