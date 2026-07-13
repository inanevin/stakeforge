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
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void make_root_row(ui::ui_context& ui, ui::widget_id_t id, const editor_widget_width_config_t& width)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui::layout_in_t&	  in	= ui.get_tree().in(id);
			apply_editor_widget_width(in, width);
			in.size_mode_y	 = ui::axis_mode_e::fixed;
			in.size_value.y	 = theme.item_height;
			in.flow			 = ui::flow_e::row;
			in.child_spacing = theme.item_spacing;
		}

		void set_input_fill(ui::ui_context& ui, ui::widget_id_t id)
		{
			ui::layout_in_t& in = ui.get_tree().in(id);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
		}

		editor_input_field_config_t make_number_config(const char* placeholder, span_t<u8*> fields, f32 increment, bool integer, editor_widget_callbacks_t callbacks = {})
		{
			return {
				.field =
					{
						.fields		= fields,
						.field_size = sizeof(f32),
						.type		= editor_input_field_field_type_e::pod_number,
					},
				.callbacks	 = callbacks,
				.placeholder = placeholder,
				.increment	 = integer ? 1.0f : increment,
				.min_value	 = integer ? -2147483648.0f : -1000000.0f,
				.max_value	 = integer ? 2147483647.0f : 1000000.0f,
				.is_integer	 = integer,
			};
		}
	}

	void editor_vec2_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec2_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec2_field");
		tree.attach(parent, _root);
		make_root_row(ui, _root, config.width);

		const char* names[2] = {"X", "Y"};
		for (u8 i = 0; i < 2; ++i)
		{
			_fields[i].reserve(config.field.fields.size > 0 ? config.field.fields.size : 1);
			_fields[i].push_back(reinterpret_cast<u8*>(&(&_value.x)[i]));
			_inputs[i].init(ui, _root, make_number_config(names[i], {.data = _fields[i].data(), .size = _fields[i].size()}, config.increment, config.integer, config.callbacks));
			set_input_fill(ui, _inputs[i].get_root());
		}

		update_field_data(config.field);
	}

	void editor_vec2_field_t::uninit()
	{
		for (u8 i = 0; i < 2; ++i)
			_inputs[1 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f};
		_field_values.resize(0);
		for (vector_t<u8*>& fields : _fields)
			fields.resize(0);
	}

	void editor_vec2_field_t::set_value(const vec2f_t& value)
	{
		_value = value;
		refresh_field_data();
	}

	void editor_vec2_field_t::set_mixed(bool)
	{
		refresh_field_data();
	}

	void editor_vec2_field_t::update_field_data(editor_vec2_field_field_t field)
	{
		if (field.fields.size > 0)
		{
			if (field.fields.data != _field_values.data())
				_field_values.assign(field.fields.data, field.fields.data + field.fields.size);
			field.fields = {.data = _field_values.data(), .size = _field_values.size()};
		}
		else
		{
			_field_values.resize(0);
		}
		_config.field					  = field;
		vec2f_t*			   fallback[] = {&_value};
		const span_t<vec2f_t*> fields	  = field.fields.size > 0 ? field.fields : span_t<vec2f_t*>{.data = fallback, .size = 1};
		for (u8 component = 0; component < 2; ++component)
		{
			_fields[component].resize(0);
			_fields[component].reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				_fields[component].push_back(reinterpret_cast<u8*>(&(&fields.data[i]->x)[component]));
			_inputs[component].update_field_data({
				.fields		= {.data = _fields[component].data(), .size = _fields[component].size()},
				.field_size = sizeof(f32),
				.type		= editor_input_field_field_type_e::pod_number,
			});
		}
	}

	void editor_vec2_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_vec3_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec3_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec3_field");
		tree.attach(parent, _root);
		make_root_row(ui, _root, config.width);

		const char* names[3] = {"X", "Y", "Z"};
		for (u8 i = 0; i < 3; ++i)
		{
			_fields[i].reserve(config.field.fields.size > 0 ? config.field.fields.size : 1);
			_fields[i].push_back(reinterpret_cast<u8*>(&(&_value.x)[i]));
			_inputs[i].init(ui, _root, make_number_config(names[i], {.data = _fields[i].data(), .size = _fields[i].size()}, config.increment, config.integer, config.callbacks));
			set_input_fill(ui, _inputs[i].get_root());
		}

		update_field_data(config.field);
	}

	void editor_vec3_field_t::uninit()
	{
		for (u8 i = 0; i < 3; ++i)
			_inputs[2 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f, 0.0f};
		_field_values.resize(0);
		for (vector_t<u8*>& fields : _fields)
			fields.resize(0);
	}

	void editor_vec3_field_t::set_value(const vec3f_t& value)
	{
		_value = value;
		refresh_field_data();
	}

	void editor_vec3_field_t::set_mixed(bool)
	{
		refresh_field_data();
	}

	void editor_vec3_field_t::update_field_data(editor_vec3_field_field_t field)
	{
		if (field.fields.size > 0)
		{
			if (field.fields.data != _field_values.data())
				_field_values.assign(field.fields.data, field.fields.data + field.fields.size);
			field.fields = {.data = _field_values.data(), .size = _field_values.size()};
		}
		else
		{
			_field_values.resize(0);
		}
		_config.field					  = field;
		vec3f_t*			   fallback[] = {&_value};
		const span_t<vec3f_t*> fields	  = field.fields.size > 0 ? field.fields : span_t<vec3f_t*>{.data = fallback, .size = 1};
		for (u8 component = 0; component < 3; ++component)
		{
			_fields[component].resize(0);
			_fields[component].reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				_fields[component].push_back(reinterpret_cast<u8*>(&(&fields.data[i]->x)[component]));
			_inputs[component].update_field_data({
				.fields		= {.data = _fields[component].data(), .size = _fields[component].size()},
				.field_size = sizeof(f32),
				.type		= editor_input_field_field_type_e::pod_number,
			});
		}
	}

	void editor_vec3_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_vec4_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec4_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec4_field");
		tree.attach(parent, _root);
		make_root_row(ui, _root, config.width);

		const char* names[4] = {"X", "Y", "Z", "W"};
		for (u8 i = 0; i < 4; ++i)
		{
			_fields[i].reserve(config.field.fields.size > 0 ? config.field.fields.size : 1);
			_fields[i].push_back(reinterpret_cast<u8*>(&(&_value.x)[i]));
			_inputs[i].init(ui, _root, make_number_config(names[i], {.data = _fields[i].data(), .size = _fields[i].size()}, config.increment, config.integer, config.callbacks));
			set_input_fill(ui, _inputs[i].get_root());
		}

		update_field_data(config.field);
	}

	void editor_vec4_field_t::uninit()
	{
		for (u8 i = 0; i < 4; ++i)
			_inputs[3 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f, 0.0f, 0.0f};
		_field_values.resize(0);
		for (vector_t<u8*>& fields : _fields)
			fields.resize(0);
	}

	void editor_vec4_field_t::set_value(const vec4f_t& value)
	{
		_value = value;
		refresh_field_data();
	}

	void editor_vec4_field_t::set_mixed(bool)
	{
		refresh_field_data();
	}

	void editor_vec4_field_t::update_field_data(editor_vec4_field_field_t field)
	{
		if (field.fields.size > 0)
		{
			if (field.fields.data != _field_values.data())
				_field_values.assign(field.fields.data, field.fields.data + field.fields.size);
			field.fields = {.data = _field_values.data(), .size = _field_values.size()};
		}
		else
		{
			_field_values.resize(0);
		}
		_config.field					  = field;
		vec4f_t*			   fallback[] = {&_value};
		const span_t<vec4f_t*> fields	  = field.fields.size > 0 ? field.fields : span_t<vec4f_t*>{.data = fallback, .size = 1};
		for (u8 component = 0; component < 4; ++component)
		{
			_fields[component].resize(0);
			_fields[component].reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				_fields[component].push_back(reinterpret_cast<u8*>(&(&fields.data[i]->x)[component]));
			_inputs[component].update_field_data({
				.fields		= {.data = _fields[component].data(), .size = _fields[component].size()},
				.field_size = sizeof(f32),
				.type		= editor_input_field_field_type_e::pod_number,
			});
		}
	}

	void editor_vec4_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_quat_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_quat_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_euler_values.push_back(vec3f_t::zero);

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "quat_field");
		tree.attach(parent, _root);
		make_root_row(ui, _root, config.width);

		const char* names[3] = {"X", "Y", "Z"};
		for (u8 i = 0; i < 3; ++i)
		{
			_fields[i].reserve(config.field.fields.size > 0 ? config.field.fields.size : 1);
			_fields[i].push_back(reinterpret_cast<u8*>(&(&_euler_values[0].x)[i]));
			editor_widget_callbacks_t callbacks = config.callbacks;
			callbacks.edit_begin				= on_euler_edit_begin;
			callbacks.edited					= on_euler_data_changed;
			callbacks.edit_submitted			= on_euler_edit_submitted;
			callbacks.user_data					= this;
			_inputs[i].init(ui, _root, make_number_config(names[i], {.data = _fields[i].data(), .size = _fields[i].size()}, config.increment, false, callbacks));
			set_input_fill(ui, _inputs[i].get_root());
		}

		update_field_data(config.field);
	}

	void editor_quat_field_t::uninit()
	{
		for (u8 i = 0; i < 3; ++i)
			_inputs[2 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_field_values.resize(0);
		_euler_values.resize(0);
		_value = {};
		for (vector_t<u8*>& fields : _fields)
			fields.resize(0);
	}

	void editor_quat_field_t::set_value(const quat_t& value)
	{
		_value = value;
		_field_values.resize(0);
		_config.field = {};
		if (_euler_values.empty())
			_euler_values.push_back(quat_t::to_euler(_value));
		else
			_euler_values[0] = quat_t::to_euler(_value);
		refresh_field_data();
	}

	void editor_quat_field_t::set_mixed(bool)
	{
		refresh_field_data();
	}

	void editor_quat_field_t::update_field_data(editor_quat_field_field_t field)
	{
		if (field.fields.size > 0)
		{
			if (field.fields.data != _field_values.data())
				_field_values.assign(field.fields.data, field.fields.data + field.fields.size);
			field.fields = {.data = _field_values.data(), .size = _field_values.size()};
			_euler_values.resize(field.fields.size);
			for (size_t i = 0; i < field.fields.size; ++i)
				_euler_values[i] = quat_t::to_euler(*field.fields.data[i]);
		}
		else
		{
			_field_values.resize(0);
			_field_values.push_back(&_value);
			if (_euler_values.empty())
				_euler_values.push_back(quat_t::to_euler(_value));
			else
				_euler_values[0] = quat_t::to_euler(_value);
			_euler_values.resize(1);
		}

		_config.field = field;
		for (u8 component = 0; component < 3; ++component)
		{
			_fields[component].resize(0);
			_fields[component].reserve(_euler_values.size());
			for (size_t i = 0; i < _euler_values.size(); ++i)
				_fields[component].push_back(reinterpret_cast<u8*>(&(&_euler_values[i].x)[component]));
			_inputs[component].update_field_data({
				.fields		= {.data = _fields[component].data(), .size = _fields[component].size()},
				.field_size = sizeof(f32),
				.type		= editor_input_field_field_type_e::pod_number,
			});
		}
	}

	void editor_quat_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_quat_field_t::modify_field()
	{
		for (size_t i = 0; i < _field_values.size(); ++i)
			*_field_values[i] = quat_t::from_euler(_euler_values[i].x, _euler_values[i].y, _euler_values[i].z);

		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	void editor_quat_field_t::on_euler_edit_begin(void* user_data)
	{
		editor_quat_field_t& field = *static_cast<editor_quat_field_t*>(user_data);
		if (field._config.callbacks.edit_begin != nullptr)
			field._config.callbacks.edit_begin(field._config.callbacks.user_data);
	}

	void editor_quat_field_t::on_euler_data_changed(void* user_data)
	{
		static_cast<editor_quat_field_t*>(user_data)->modify_field();
	}

	void editor_quat_field_t::on_euler_edit_submitted(void* user_data)
	{
		editor_quat_field_t& field = *static_cast<editor_quat_field_t*>(user_data);
		if (field._config.callbacks.edit_submitted != nullptr)
			field._config.callbacks.edit_submitted(field._config.callbacks.user_data);
	}
}
