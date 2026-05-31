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
#include "ui/widgets/editor_widget_reflect_type.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <cstring>

namespace sfg
{
	namespace
	{
		void add_unknown_label(ui::ui_context& ui, ui::widget_id_t parent)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			ui::paint_layer_t&	  paint = ui.get_paint();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t label = ui.allocate_widget();
			ui.set_widget_debug_name(label, "reflect_type_unknown");
			tree.attach(parent, label);
			tree.draw_order(label) = tree.draw_order_const(parent);

			ui::layout_in_t& label_in = tree.in(label);
			label_in.flags			  = ui::wf_visible;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(label, "unknown");
			paint.set_text(
				label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		void apply_reflected_number_config(editor_input_field_config_t& config, const reflected_field_desc_t& field, bool integer)
		{
			config.type		 = (field.flags & reflected_field_flags_clamped) != 0 ? editor_input_field_type_e::number_slider : editor_input_field_type_e::number;
			config.min_value = field.min;
			config.max_value = field.max;
			config.increment = integer ? 1.0f : 0.1f;
			config.integer	 = integer;
		}

		const void* get_reflected_field_ptr(const void* object, const reflected_field_desc_t& field)
		{
			return static_cast<const u8*>(object) + field.offset;
		}

		void* get_reflected_field_ptr(void* object, const reflected_field_desc_t& field)
		{
			return static_cast<u8*>(object) + field.offset;
		}

		template <typename T> T read_reflected_value(const void* object, const reflected_field_desc_t& field)
		{
			T value = {};
			if (field.get != nullptr)
			{
				field.get(object, field, &value, field.user_data);
				return value;
			}
			return *static_cast<const T*>(get_reflected_field_ptr(object, field));
		}

		template <typename T> void write_reflected_value(void* object, const reflected_field_desc_t& field, const T& value)
		{
			if ((field.flags & reflected_field_flags_read_only) != 0)
				return;
			if (field.set != nullptr)
			{
				field.set(object, field, &value, field.user_data);
				return;
			}
			*static_cast<T*>(get_reflected_field_ptr(object, field)) = value;
		}

		f32 read_reflected_number(const void* object, const reflected_field_desc_t& field)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32:
				return read_reflected_value<f32>(object, field);
			case reflected_value_type_e::i32:
				return static_cast<f32>(read_reflected_value<i32>(object, field));
			case reflected_value_type_e::u32:
				return static_cast<f32>(read_reflected_value<u32>(object, field));
			case reflected_value_type_e::u8:
				return static_cast<f32>(read_reflected_value<u8>(object, field));
			default:
				return 0.0f;
			}
		}

		void write_reflected_number(void* object, const reflected_field_desc_t& field, f32 value)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32:
				write_reflected_value(object, field, value);
				break;
			case reflected_value_type_e::i32:
				write_reflected_value(object, field, static_cast<i32>(value));
				break;
			case reflected_value_type_e::u32:
				write_reflected_value(object, field, static_cast<u32>(value < 0.0f ? 0.0f : value));
				break;
			case reflected_value_type_e::u8:
				write_reflected_value(object, field, static_cast<u8>(value < 0.0f ? 0.0f : (value > 255.0f ? 255.0f : value)));
				break;
			default:
				break;
			}
		}

		bool read_reflected_bool(const void* object, const reflected_field_desc_t& field)
		{
			bool value = false;
			if (field.get != nullptr)
			{
				field.get(object, field, &value, field.user_data);
				return value;
			}
			return *static_cast<const u8*>(get_reflected_field_ptr(object, field)) != 0;
		}

		void write_reflected_bool(void* object, const reflected_field_desc_t& field, bool value)
		{
			if ((field.flags & reflected_field_flags_read_only) != 0)
				return;
			if (field.set != nullptr)
			{
				field.set(object, field, &value, field.user_data);
				return;
			}
			*static_cast<u8*>(get_reflected_field_ptr(object, field)) = value ? 1 : 0;
		}

		const char* read_reflected_text(const void* object, const reflected_field_desc_t& field)
		{
			const char* value = nullptr;
			if (field.get != nullptr)
			{
				field.get(object, field, &value, field.user_data);
				return value != nullptr ? value : "";
			}
			return static_cast<const char*>(get_reflected_field_ptr(object, field));
		}

		void write_reflected_text(void* object, const reflected_field_desc_t& field, const char* value)
		{
			if ((field.flags & reflected_field_flags_read_only) != 0)
				return;
			if (field.set != nullptr)
			{
				field.set(object, field, value, field.user_data);
				return;
			}

			SFG_ASSERT(field.size > 0);
			char*		 dst	 = static_cast<char*>(get_reflected_field_ptr(object, field));
			const char*	 src	 = value != nullptr ? value : "";
			const size_t max_len = static_cast<size_t>(field.size - 1);
			const size_t len	 = std::strlen(src) < max_len ? std::strlen(src) : max_len;
			std::memcpy(dst, src, len);
			dst[len] = '\0';
		}

		i64 read_reflected_enum(const void* object, const reflected_field_desc_t& field)
		{
			const u32 raw = read_reflected_value<u32>(object, field);
			for (u32 i = 0; i < field.enum_values.size; ++i)
			{
				const i64 value = field.enum_values.data[i].value;
				if (value == static_cast<i64>(raw) || value == static_cast<i64>(static_cast<i32>(raw)))
					return value;
			}
			return static_cast<i64>(raw);
		}

		void write_reflected_enum(void* object, const reflected_field_desc_t& field, i64 value)
		{
			write_reflected_value(object, field, static_cast<u32>(value));
		}
	}

	void editor_widget_reflect_type_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reflect_type");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fill;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
	}

	void editor_widget_reflect_type_t::uninit()
	{
		clear_reflected_controls();
		_ui->deallocate_widget(_root);

		_ui		 = nullptr;
		_object	 = nullptr;
		_type_id = 0;
		_root	 = NULL_WIDGET;
	}

	void editor_widget_reflect_type_t::set_reflected_obj(void* object, sid_t type_id)
	{
		_object	 = object;
		_type_id = type_id;

		clear_reflected_controls();

		const reflected_type_desc_t* type = reflection_registry_t::get().find_type(type_id);
		if (type == nullptr)
			return;

		_controls.reserve(type->fields.size);
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			const editor_property_row_t	  row	= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, field.display_name != nullptr ? field.display_name : field.name);
			_rows.push_back(row);
			_controls.push_back({.owner = this, .field = &field});
			reflected_control_t* control = &_controls.back();

			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				editor_input_field_t*		input  = new editor_input_field_t();
				editor_input_field_config_t config = {};
				apply_reflected_number_config(config, field, false);
				config.number_value		 = read_reflected_number(_object, field);
				config.on_number_changed = on_number_changed;
				config.user_data		 = control;
				input->init(*_ui, row.right, config);
				_input_fields.push_back(input);
				break;
			}
			case reflected_value_type_e::i32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::u8: {
				editor_input_field_t*		input  = new editor_input_field_t();
				editor_input_field_config_t config = {};
				apply_reflected_number_config(config, field, true);
				config.number_value		 = read_reflected_number(_object, field);
				config.on_number_changed = on_number_changed;
				config.user_data		 = control;
				input->init(*_ui, row.right, config);
				_input_fields.push_back(input);
				break;
			}
			case reflected_value_type_e::bool8: {
				editor_checkbox_t*		 checkbox = new editor_checkbox_t();
				editor_checkbox_config_t config	  = {};
				config.checked					  = read_reflected_bool(_object, field);
				config.on_changed				  = on_checkbox_changed;
				config.user_data				  = control;
				checkbox->init(*_ui, row.right, config);
				_checkboxes.push_back(checkbox);
				break;
			}
			case reflected_value_type_e::vec2: {
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				config.value					  = read_reflected_value<vec2f_t>(_object, field);
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, row.right, config);
				_vec2_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::vec3: {
				editor_vec3_field_t*	   vec	  = new editor_vec3_field_t();
				editor_vec3_field_config_t config = {};
				config.value					  = read_reflected_value<vec3f_t>(_object, field);
				config.on_changed				  = on_vec3_changed;
				config.user_data				  = control;
				vec->init(*_ui, row.right, config);
				_vec3_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::vec4: {
				editor_vec4_field_t*	   vec	  = new editor_vec4_field_t();
				editor_vec4_field_config_t config = {};
				config.value					  = read_reflected_value<vec4f_t>(_object, field);
				config.on_changed				  = on_vec4_changed;
				config.user_data				  = control;
				vec->init(*_ui, row.right, config);
				_vec4_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::color: {
				editor_color_field_t*		color  = new editor_color_field_t();
				editor_color_field_config_t config = {};
				config.color					   = read_reflected_value<vec4f_t>(_object, field);
				config.on_changed				   = on_color_changed;
				config.user_data				   = control;
				color->init(*_ui, row.right, config);
				_color_fields.push_back(color);
				break;
			}
			case reflected_value_type_e::string: {
				editor_input_field_t*		input  = new editor_input_field_t();
				editor_input_field_config_t config = {};
				config.type						   = editor_input_field_type_e::text;
				config.text_value				   = read_reflected_text(_object, field);
				config.on_text_changed			   = on_text_changed;
				config.user_data				   = control;
				input->init(*_ui, row.right, config);
				_input_fields.push_back(input);
				break;
			}
			case reflected_value_type_e::enum32: {
				_dropdowns.push_back({});
				enum_control_t& enum_control = _dropdowns.back();
				enum_control.items.reserve(field.enum_values.size);
				for (u32 enum_index = 0; enum_index < field.enum_values.size; ++enum_index)
				{
					const reflected_enum_value_desc_t& value = field.enum_values.data[enum_index];
					enum_control.items.push_back({.text = value.display_name != nullptr ? value.display_name : value.name, .value = static_cast<u16>(enum_index)});
				}

				enum_control.dropdown			= new editor_dropdown_t();
				editor_dropdown_config_t config = {};
				config.items					= enum_control.items.data();
				config.item_count				= static_cast<u16>(enum_control.items.size());
				config.width					= editor_dropdown_width_e::parent_relative;
				config.title_from_selection		= true;
				config.selected					= on_enum_selected;
				config.pressed					= on_enum_pressed;
				config.user_data				= control;
				enum_control.dropdown->init(*_ui, row.right, config);
				break;
			}
			default: {
				add_unknown_label(*_ui, row.right);
				break;
			}
			}
		}
	}

	void editor_widget_reflect_type_t::on_number_changed(f32 value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_number(control.owner->_object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_text_changed(const char* value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_text(control.owner->_object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_checkbox_changed(bool checked, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_bool(control.owner->_object, *control.field, checked);
	}

	void editor_widget_reflect_type_t::on_color_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.owner->_object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.owner->_object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.owner->_object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.owner->_object, *control.field, value);
	}

	u16 editor_widget_reflect_type_t::on_enum_selected(void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		const i64			 value	 = read_reflected_enum(control.owner->_object, *control.field);
		for (u16 i = 0; i < control.field->enum_values.size; ++i)
		{
			if (control.field->enum_values.data[i].value == value)
				return i;
		}
		return 0;
	}

	void editor_widget_reflect_type_t::on_enum_pressed(u16 value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		SFG_ASSERT(value < control.field->enum_values.size);
		write_reflected_enum(control.owner->_object, *control.field, control.field->enum_values.data[value].value);
	}

	void editor_widget_reflect_type_t::clear_reflected_controls()
	{
		for (size_t i = _dropdowns.size(); i > 0; --i)
		{
			enum_control_t& control = _dropdowns[i - 1];
			control.dropdown->uninit();
			delete control.dropdown;
		}
		_dropdowns.resize(0);

		for (size_t i = _vec4_fields.size(); i > 0; --i)
		{
			_vec4_fields[i - 1]->uninit();
			delete _vec4_fields[i - 1];
		}
		_vec4_fields.resize(0);

		for (size_t i = _vec3_fields.size(); i > 0; --i)
		{
			_vec3_fields[i - 1]->uninit();
			delete _vec3_fields[i - 1];
		}
		_vec3_fields.resize(0);

		for (size_t i = _vec2_fields.size(); i > 0; --i)
		{
			_vec2_fields[i - 1]->uninit();
			delete _vec2_fields[i - 1];
		}
		_vec2_fields.resize(0);

		for (size_t i = _color_fields.size(); i > 0; --i)
		{
			_color_fields[i - 1]->uninit();
			delete _color_fields[i - 1];
		}
		_color_fields.resize(0);

		for (size_t i = _checkboxes.size(); i > 0; --i)
		{
			_checkboxes[i - 1]->uninit();
			delete _checkboxes[i - 1];
		}
		_checkboxes.resize(0);

		for (size_t i = _input_fields.size(); i > 0; --i)
		{
			_input_fields[i - 1]->uninit();
			delete _input_fields[i - 1];
		}
		_input_fields.resize(0);

		for (size_t i = _rows.size(); i > 0; --i)
			_ui->deallocate_widget(_rows[i - 1].row);
		_rows.resize(0);
		_controls.resize(0);
	}
}
