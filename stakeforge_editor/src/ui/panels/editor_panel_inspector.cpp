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
#include "ui/panels/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
	}

	void editor_panel_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "inspector_input_column");
		tree.attach(_root, _column);

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		column_in.size_value	   = {1.0f, 0.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		editor_input_field_config_t input_config = {};
		input_config.placeholder				 = "Text field";
		input_config.text_value					 = "Stakeforge";
		input_config.type						 = editor_input_field_type_e::text;
		input_config.on_text_changed			 = on_text_changed;
		input_config.user_data					 = this;
		_text_input.init(ui, _column, input_config);

		input_config				   = {};
		input_config.placeholder	   = "Float number";
		input_config.type			   = editor_input_field_type_e::number;
		input_config.number_value	   = _float_value;
		input_config.increment		   = 0.1f;
		input_config.on_number_changed = on_number_changed;
		input_config.user_data		   = &_float_value;
		_float_input.init(ui, _column, input_config);

		input_config				   = {};
		input_config.placeholder	   = "Integer number";
		input_config.type			   = editor_input_field_type_e::number;
		input_config.number_value	   = _int_value;
		input_config.increment		   = 1.0f;
		input_config.integer		   = true;
		input_config.on_number_changed = on_number_changed;
		input_config.user_data		   = &_int_value;
		_int_input.init(ui, _column, input_config);

		input_config				   = {};
		input_config.placeholder	   = "Slider";
		input_config.type			   = editor_input_field_type_e::number_slider;
		input_config.number_value	   = _slider_value;
		input_config.increment		   = 0.01f;
		input_config.min_value		   = 0.0f;
		input_config.max_value		   = 1.0f;
		input_config.on_number_changed = on_number_changed;
		input_config.user_data		   = &_slider_value;
		_slider_input.init(ui, _column, input_config);

		input_config				   = {};
		input_config.placeholder	   = "Integer slider";
		input_config.type			   = editor_input_field_type_e::number_slider;
		input_config.number_value	   = _int_slider_value;
		input_config.increment		   = 1.0f;
		input_config.min_value		   = 0.0f;
		input_config.max_value		   = 10.0f;
		input_config.integer		   = true;
		input_config.on_number_changed = on_number_changed;
		input_config.user_data		   = &_int_slider_value;
		_int_slider_input.init(ui, _column, input_config);

		editor_button_config_t button_config = {};
		button_config.text					 = "Button";
		_button.init(ui, _column, button_config);

		editor_checkbox_config_t checkbox_config = {};
		checkbox_config.checked					 = _checkbox_value;
		checkbox_config.on_changed				 = on_checkbox_changed;
		checkbox_config.user_data				 = &_checkbox_value;
		_checkbox.init(ui, _column, checkbox_config);

		editor_color_field_config_t color_config = {};
		color_config.color						 = _color_value;
		color_config.on_changed					 = on_color_changed;
		color_config.user_data					 = &_color_value;
		_color_field.init(ui, _column, color_config);

		editor_vec2_field_config_t vec2_config = {};
		vec2_config.value					   = _vec2_value;
		vec2_config.increment				   = 0.1f;
		vec2_config.on_changed				   = on_vec2_changed;
		vec2_config.user_data				   = &_vec2_value;
		_vec2_field.init(ui, _column, vec2_config);

		editor_vec3_field_config_t vec3_config = {};
		vec3_config.value					   = _vec3_value;
		vec3_config.increment				   = 0.1f;
		vec3_config.on_changed				   = on_vec3_changed;
		vec3_config.user_data				   = &_vec3_value;
		_vec3_field.init(ui, _column, vec3_config);

		editor_vec4_field_config_t vec4_config = {};
		vec4_config.value					   = _vec4_value;
		vec4_config.increment				   = 0.1f;
		vec4_config.on_changed				   = on_vec4_changed;
		vec4_config.user_data				   = &_vec4_value;
		_vec4_field.init(ui, _column, vec4_config);
	}

	void editor_panel_inspector_t::uninit()
	{
		_vec4_field.uninit();
		_vec3_field.uninit();
		_vec2_field.uninit();
		_color_field.uninit();
		_checkbox.uninit();
		_button.uninit();
		_int_slider_input.uninit();
		_slider_input.uninit();
		_int_input.uninit();
		_float_input.uninit();
		_text_input.uninit();
		editor_panel_t::uninit();

		_column = NULL_WIDGET;
	}

	void editor_panel_inspector_t::on_text_changed(const char*, void*)
	{
	}

	void editor_panel_inspector_t::on_number_changed(f32 value, void* user_data)
	{
		*static_cast<f32*>(user_data) = value;
	}

	void editor_panel_inspector_t::on_checkbox_changed(bool checked, void* user_data)
	{
		*static_cast<bool*>(user_data) = checked;
	}

	void editor_panel_inspector_t::on_color_changed(const vec4f_t& value, void* user_data)
	{
		*static_cast<vec4f_t*>(user_data) = value;
	}

	void editor_panel_inspector_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		*static_cast<vec2f_t*>(user_data) = value;
	}

	void editor_panel_inspector_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		*static_cast<vec3f_t*>(user_data) = value;
	}

	void editor_panel_inspector_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		*static_cast<vec4f_t*>(user_data) = value;
	}
}
