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
#include <sfg/common/hashing.hpp>
#include <sfg/data/string.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>

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

		void center_property_row_control(ui::ui_context& ui, ui::widget_id_t id)
		{
			ui::layout_in_t& in = ui.get_tree().in(id);
			if (in.size_mode_x == ui::axis_mode_e::parent_relative)
				in.size_mode_x = ui::axis_mode_e::fill;
			in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
			in.pos_value.y = 0.5f;
			in.anchor_y	   = ui::anchor_e::center;
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
			u64 raw = 0;
			if (field.get != nullptr)
			{
				u32 value = 0;
				field.get(object, field, &value, field.user_data);
				raw = value;
			}
			else
			{
				const void* ptr = get_reflected_field_ptr(object, field);
				switch (field.size)
				{
				case sizeof(u8):
					raw = *static_cast<const u8*>(ptr);
					break;
				case sizeof(u16):
					raw = *static_cast<const u16*>(ptr);
					break;
				case sizeof(u64):
					raw = *static_cast<const u64*>(ptr);
					break;
				default:
					raw = *static_cast<const u32*>(ptr);
					break;
				}
			}

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
			if ((field.flags & reflected_field_flags_read_only) != 0)
				return;
			if (field.set != nullptr)
			{
				const u32 raw = static_cast<u32>(value);
				field.set(object, field, &raw, field.user_data);
				return;
			}

			void* ptr = get_reflected_field_ptr(object, field);
			switch (field.size)
			{
			case sizeof(u8):
				*static_cast<u8*>(ptr) = static_cast<u8>(value);
				break;
			case sizeof(u16):
				*static_cast<u16*>(ptr) = static_cast<u16>(value);
				break;
			case sizeof(u64):
				*static_cast<u64*>(ptr) = static_cast<u64>(value);
				break;
			default:
				*static_cast<u32*>(ptr) = static_cast<u32>(value);
				break;
			}
		}

		bool is_vector_field(const reflected_field_desc_t& field)
		{
			return field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::static_vector;
		}

		bool read_string_value(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			*static_cast<const char**>(out_value) = static_cast<const string_t*>(object)->c_str();
			return true;
		}

		bool write_string_value(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			*static_cast<string_t*>(object) = static_cast<const char*>(value);
			return true;
		}

		const reflected_field_desc_t& get_f32_item_field()
		{
			static const reflected_field_desc_t field = {
				.name = "value",
				.type = reflected_value_type_e::f32,
				.size = sizeof(f32),
			};
			return field;
		}

		const reflected_field_desc_t& get_string_item_field()
		{
			static const reflected_field_desc_t field = {
				.get  = read_string_value,
				.set  = write_string_value,
				.name = "value",
				.type = reflected_value_type_e::string,
				.size = sizeof(string_t),
			};
			return field;
		}

		template <typename T> vector_t<T>& get_reflected_vector(void* object, const reflected_field_desc_t& field)
		{
			return *static_cast<vector_t<T>*>(get_reflected_field_ptr(object, field));
		}

		template <typename T> const vector_t<T>& get_reflected_vector(const void* object, const reflected_field_desc_t& field)
		{
			return *static_cast<const vector_t<T>*>(get_reflected_field_ptr(object, field));
		}

		template <typename T> size_t get_static_vector_head_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = sizeof(T) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		template <typename T> T* get_reflected_static_vector_data(void* object, const reflected_field_desc_t& field)
		{
			return std::launder(reinterpret_cast<T*>(get_reflected_field_ptr(object, field)));
		}

		template <typename T> const T* get_reflected_static_vector_data(const void* object, const reflected_field_desc_t& field)
		{
			return std::launder(reinterpret_cast<const T*>(get_reflected_field_ptr(object, field)));
		}

		template <typename T> size_t& get_reflected_static_vector_size(void* object, const reflected_field_desc_t& field)
		{
			return *reinterpret_cast<size_t*>(static_cast<u8*>(get_reflected_field_ptr(object, field)) + get_static_vector_head_offset<T>(field));
		}

		template <typename T> const size_t& get_reflected_static_vector_size(const void* object, const reflected_field_desc_t& field)
		{
			return *reinterpret_cast<const size_t*>(static_cast<const u8*>(get_reflected_field_ptr(object, field)) + get_static_vector_head_offset<T>(field));
		}

		template <typename T> void clear_reflected_static_vector(void* object, const reflected_field_desc_t& field)
		{
			T*		data = get_reflected_static_vector_data<T>(object, field);
			size_t& size = get_reflected_static_vector_size<T>(object, field);
			while (size > 0)
			{
				--size;
				std::destroy_at(data + size);
			}
		}

		template <typename T> void add_reflected_static_vector_item(void* object, const reflected_field_desc_t& field)
		{
			T*		data = get_reflected_static_vector_data<T>(object, field);
			size_t& size = get_reflected_static_vector_size<T>(object, field);
			if (size == field.capacity)
				return;
			std::construct_at(data + size);
			++size;
		}

		template <typename T> void remove_reflected_static_vector_item(void* object, const reflected_field_desc_t& field, u32 index)
		{
			T*		data = get_reflected_static_vector_data<T>(object, field);
			size_t& size = get_reflected_static_vector_size<T>(object, field);
			SFG_ASSERT(index < size);
			for (size_t i = index; i < size - 1; ++i)
				data[i] = std::move(data[i + 1]);
			--size;
			std::destroy_at(data + size);
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
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
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
		_vector_states.resize(0);
	}

	void editor_widget_reflect_type_t::set_reflected_obj(void* object, sid_t type_id)
	{
		_object	 = object;
		_type_id = type_id;
		_vector_states.resize(0);
		rebuild_reflected_controls();
	}

	void editor_widget_reflect_type_t::rebuild_reflected_controls()
	{
		clear_reflected_controls();

		const reflected_type_desc_t* type = reflection_registry_t::get().find_type(_type_id);
		if (type == nullptr)
			return;

		u32 control_count = static_cast<u32>(type->fields.size);
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if (is_vector_field(field) && is_vector_unfolded(field.id))
				control_count += get_vector_item_count(field);
		}

		_controls.reserve(control_count);
		_vector_controls.reserve(type->fields.size);
		_vector_item_controls.reserve(control_count);
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			const char*					  label = field.display_name != nullptr ? field.display_name : field.name;
			if (is_vector_field(field))
			{
				install_vector_field(field, label);
				continue;
			}

			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label);
			_rows.push_back(row);
			install_reflected_control(row.right, field, _object);
		}
	}

	void editor_widget_reflect_type_t::install_reflected_control(ui::widget_id_t parent, const reflected_field_desc_t& field, void* object)
	{
		_controls.push_back({.owner = this, .field = &field, .object = object});
		reflected_control_t* control = &_controls.back();

		switch (field.type)
		{
		case reflected_value_type_e::f32: {
			editor_input_field_t*		input  = new editor_input_field_t();
			editor_input_field_config_t config = {};
			apply_reflected_number_config(config, field, false);
			config.number_value		 = read_reflected_number(object, field);
			config.on_number_changed = on_number_changed;
			config.user_data		 = control;
			input->init(*_ui, parent, config);
			center_property_row_control(*_ui, input->get_root());
			_input_fields.push_back(input);
			break;
		}
		case reflected_value_type_e::i32:
		case reflected_value_type_e::u32:
		case reflected_value_type_e::u8: {
			editor_input_field_t*		input  = new editor_input_field_t();
			editor_input_field_config_t config = {};
			apply_reflected_number_config(config, field, true);
			config.number_value		 = read_reflected_number(object, field);
			config.on_number_changed = on_number_changed;
			config.user_data		 = control;
			input->init(*_ui, parent, config);
			center_property_row_control(*_ui, input->get_root());
			_input_fields.push_back(input);
			break;
		}
		case reflected_value_type_e::bool8: {
			editor_checkbox_t*		 checkbox = new editor_checkbox_t();
			editor_checkbox_config_t config	  = {};
			config.checked					  = read_reflected_bool(object, field);
			config.on_changed				  = on_checkbox_changed;
			config.user_data				  = control;
			checkbox->init(*_ui, parent, config);
			center_property_row_control(*_ui, checkbox->get_root());
			_checkboxes.push_back(checkbox);
			break;
		}
		case reflected_value_type_e::vec2: {
			editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
			editor_vec2_field_config_t config = {};
			config.value					  = read_reflected_value<vec2f_t>(object, field);
			config.on_changed				  = on_vec2_changed;
			config.user_data				  = control;
			vec->init(*_ui, parent, config);
			center_property_row_control(*_ui, vec->get_root());
			_vec2_fields.push_back(vec);
			break;
		}
		case reflected_value_type_e::vec3: {
			editor_vec3_field_t*	   vec	  = new editor_vec3_field_t();
			editor_vec3_field_config_t config = {};
			config.value					  = read_reflected_value<vec3f_t>(object, field);
			config.on_changed				  = on_vec3_changed;
			config.user_data				  = control;
			vec->init(*_ui, parent, config);
			center_property_row_control(*_ui, vec->get_root());
			_vec3_fields.push_back(vec);
			break;
		}
		case reflected_value_type_e::vec4: {
			editor_vec4_field_t*	   vec	  = new editor_vec4_field_t();
			editor_vec4_field_config_t config = {};
			config.value					  = read_reflected_value<vec4f_t>(object, field);
			config.on_changed				  = on_vec4_changed;
			config.user_data				  = control;
			vec->init(*_ui, parent, config);
			center_property_row_control(*_ui, vec->get_root());
			_vec4_fields.push_back(vec);
			break;
		}
		case reflected_value_type_e::color: {
			editor_color_field_t*		color  = new editor_color_field_t();
			editor_color_field_config_t config = {};
			config.color					   = read_reflected_value<vec4f_t>(object, field);
			config.on_changed				   = on_color_changed;
			config.user_data				   = control;
			color->init(*_ui, parent, config);
			center_property_row_control(*_ui, color->get_root());
			_color_fields.push_back(color);
			break;
		}
		case reflected_value_type_e::string: {
			editor_input_field_t*		input  = new editor_input_field_t();
			editor_input_field_config_t config = {};
			config.type						   = editor_input_field_type_e::text;
			config.text_value				   = read_reflected_text(object, field);
			config.on_text_changed			   = on_text_changed;
			config.user_data				   = control;
			input->init(*_ui, parent, config);
			center_property_row_control(*_ui, input->get_root());
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
			enum_control.dropdown->init(*_ui, parent, config);
			center_property_row_control(*_ui, enum_control.dropdown->get_root());
			break;
		}
		default: {
			add_unknown_label(*_ui, parent);
			break;
		}
		}
	}

	void editor_widget_reflect_type_t::install_vector_field(const reflected_field_desc_t& field, const char* label)
	{
		const bool						   unfolded	  = is_vector_unfolded(field.id);
		const u32						   item_count = get_vector_item_count(field);
		const editor_vector_property_row_t vector_row = editor_misc_widgets_t::make_vector_property_row_with_label(*_ui, _root, label, item_count, unfolded);
		_rows.push_back(vector_row.row);

		_vector_controls.push_back({.owner = this, .field = &field});
		vector_control_t* control = &_vector_controls.back();

		ui::listener_bundle_t listener = {};
		listener.user_data			   = control;
		listener.on_click			   = on_vector_header_click;
		_ui->get_input().set_listener(vector_row.dropdown_button, listener);
		_ui->get_input().set_listener(vector_row.label, listener);

		listener.on_click = on_vector_reset_click;
		_ui->get_input().set_listener(vector_row.reset_button, listener);

		listener.on_click = on_vector_add_click;
		_ui->get_input().set_listener(vector_row.add_button, listener);

		if (!unfolded)
			return;

		if (field.sub_type_id == "f32"_hs)
		{
			const u32 item_count = get_vector_item_count(field);
			f32*	  values	 = field.type == reflected_value_type_e::static_vector ? get_reflected_static_vector_data<f32>(_object, field) : get_reflected_vector<f32>(_object, field).data();
			for (u32 i = 0; i < item_count; ++i)
			{
				char item_label[32] = {};
				std::snprintf(item_label, sizeof(item_label), "[%u]", i);
				const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, item_label, true, true);
				_rows.push_back(row);
				_vector_item_controls.push_back({.owner = this, .field = &field, .index = i});
				ui::listener_bundle_t remove_listener = {};
				remove_listener.user_data			  = &_vector_item_controls.back();
				remove_listener.on_click			  = on_vector_item_remove_click;
				_ui->get_input().set_listener(row.remove_button, remove_listener);
				install_reflected_control(row.right, get_f32_item_field(), &values[i]);
			}
			return;
		}

		if (field.sub_type_id == "string"_hs)
		{
			const u32 item_count = get_vector_item_count(field);
			string_t* values	 = field.type == reflected_value_type_e::static_vector ? get_reflected_static_vector_data<string_t>(_object, field) : get_reflected_vector<string_t>(_object, field).data();
			for (u32 i = 0; i < item_count; ++i)
			{
				char item_label[32] = {};
				std::snprintf(item_label, sizeof(item_label), "[%u]", i);
				const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, item_label, true, true);
				_rows.push_back(row);
				_vector_item_controls.push_back({.owner = this, .field = &field, .index = i});
				ui::listener_bundle_t remove_listener = {};
				remove_listener.user_data			  = &_vector_item_controls.back();
				remove_listener.on_click			  = on_vector_item_remove_click;
				_ui->get_input().set_listener(row.remove_button, remove_listener);
				install_reflected_control(row.right, get_string_item_field(), &values[i]);
			}
		}
	}

	u32 editor_widget_reflect_type_t::get_vector_item_count(const reflected_field_desc_t& field) const
	{
		if (field.sub_type_id == "f32"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				return static_cast<u32>(get_reflected_static_vector_size<f32>(_object, field));
			return static_cast<u32>(get_reflected_vector<f32>(_object, field).size());
		}
		if (field.sub_type_id == "string"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				return static_cast<u32>(get_reflected_static_vector_size<string_t>(_object, field));
			return static_cast<u32>(get_reflected_vector<string_t>(_object, field).size());
		}
		return 0;
	}

	bool editor_widget_reflect_type_t::is_vector_unfolded(sid_t field_id) const
	{
		for (const vector_fold_state_t& state : _vector_states)
		{
			if (state.field_id == field_id)
				return state.unfolded;
		}
		return false;
	}

	void editor_widget_reflect_type_t::toggle_vector_unfolded(sid_t field_id)
	{
		for (vector_fold_state_t& state : _vector_states)
		{
			if (state.field_id == field_id)
			{
				state.unfolded = !state.unfolded;
				rebuild_reflected_controls();
				return;
			}
		}

		_vector_states.push_back({.field_id = field_id, .unfolded = true});
		rebuild_reflected_controls();
	}

	void editor_widget_reflect_type_t::reset_vector_field(const reflected_field_desc_t& field)
	{
		if ((field.flags & reflected_field_flags_read_only) != 0)
			return;

		if (field.sub_type_id == "f32"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				clear_reflected_static_vector<f32>(_object, field);
			else
				get_reflected_vector<f32>(_object, field).resize(0);
			rebuild_reflected_controls();
			return;
		}

		if (field.sub_type_id == "string"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				clear_reflected_static_vector<string_t>(_object, field);
			else
				get_reflected_vector<string_t>(_object, field).resize(0);
			rebuild_reflected_controls();
		}
	}

	void editor_widget_reflect_type_t::add_vector_item(const reflected_field_desc_t& field)
	{
		if ((field.flags & reflected_field_flags_read_only) != 0)
			return;

		if (field.sub_type_id == "f32"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				add_reflected_static_vector_item<f32>(_object, field);
			else
				get_reflected_vector<f32>(_object, field).push_back(0.0f);
			rebuild_reflected_controls();
			return;
		}

		if (field.sub_type_id == "string"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
				add_reflected_static_vector_item<string_t>(_object, field);
			else
				get_reflected_vector<string_t>(_object, field).push_back(string_t{});
			rebuild_reflected_controls();
		}
	}

	void editor_widget_reflect_type_t::remove_vector_item(const reflected_field_desc_t& field, u32 index)
	{
		if ((field.flags & reflected_field_flags_read_only) != 0)
			return;

		if (field.sub_type_id == "f32"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
			{
				remove_reflected_static_vector_item<f32>(_object, field, index);
			}
			else
			{
				vector_t<f32>& values = get_reflected_vector<f32>(_object, field);
				values.erase(values.begin() + index);
			}
			rebuild_reflected_controls();
			return;
		}

		if (field.sub_type_id == "string"_hs)
		{
			if (field.type == reflected_value_type_e::static_vector)
			{
				remove_reflected_static_vector_item<string_t>(_object, field, index);
			}
			else
			{
				vector_t<string_t>& values = get_reflected_vector<string_t>(_object, field);
				values.erase(values.begin() + index);
			}
			rebuild_reflected_controls();
		}
	}

	void editor_widget_reflect_type_t::on_number_changed(f32 value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_number(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_text_changed(const char* value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_text(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_checkbox_changed(bool checked, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_bool(control.object, *control.field, checked);
	}

	void editor_widget_reflect_type_t::on_color_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		write_reflected_value(control.object, *control.field, value);
	}

	u16 editor_widget_reflect_type_t::on_enum_selected(void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		const i64			 value	 = read_reflected_enum(control.object, *control.field);
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
		write_reflected_enum(control.object, *control.field, control.field->enum_values.data[value].value);
	}

	void editor_widget_reflect_type_t::on_vector_header_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		vector_control_t& control = *static_cast<vector_control_t*>(user_data);
		control.owner->toggle_vector_unfolded(control.field->id);
	}

	void editor_widget_reflect_type_t::on_vector_reset_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		vector_control_t& control = *static_cast<vector_control_t*>(user_data);
		control.owner->reset_vector_field(*control.field);
	}

	void editor_widget_reflect_type_t::on_vector_add_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		vector_control_t& control = *static_cast<vector_control_t*>(user_data);
		control.owner->add_vector_item(*control.field);
	}

	void editor_widget_reflect_type_t::on_vector_item_remove_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		vector_item_control_t& control = *static_cast<vector_item_control_t*>(user_data);
		control.owner->remove_vector_item(*control.field, control.index);
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
		_vector_controls.resize(0);
		_vector_item_controls.resize(0);
	}
}
