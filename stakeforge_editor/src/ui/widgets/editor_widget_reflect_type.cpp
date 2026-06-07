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
#include <sfg/io/assert.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec3u.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/math/vec4u.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
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

		u32 to_reflected_u32(f32 value)
		{
			return static_cast<u32>(value < 0.0f ? 0.0f : value);
		}

		vec2f_t to_vec2f(const vec2u_t& value)
		{
			return {static_cast<f32>(value.x), static_cast<f32>(value.y)};
		}

		vec2f_t to_vec2f(const vec2u16_t& value)
		{
			return {static_cast<f32>(value.x), static_cast<f32>(value.y)};
		}

		vec3f_t to_vec3f(const vec3u_t& value)
		{
			return {static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z)};
		}

		vec4f_t to_vec4f(const vec4u_t& value)
		{
			return {static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z), static_cast<f32>(value.w)};
		}

		vec2u_t to_vec2u(const vec2f_t& value)
		{
			return {to_reflected_u32(value.x), to_reflected_u32(value.y)};
		}

		vec2u16_t to_vec2u16(const vec2f_t& value)
		{
			return {static_cast<u16>(value.x < 0.0f ? 0.0f : (value.x > 65535.0f ? 65535.0f : value.x)), static_cast<u16>(value.y < 0.0f ? 0.0f : (value.y > 65535.0f ? 65535.0f : value.y))};
		}

		vec3u_t to_vec3u(const vec3f_t& value)
		{
			return {to_reflected_u32(value.x), to_reflected_u32(value.y), to_reflected_u32(value.z)};
		}

		vec4u_t to_vec4u(const vec4f_t& value)
		{
			return {to_reflected_u32(value.x), to_reflected_u32(value.y), to_reflected_u32(value.z), to_reflected_u32(value.w)};
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
			if (field.size == sizeof(string_t))
				return static_cast<const string_t*>(get_reflected_field_ptr(object, field))->c_str();
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

			if (field.size == sizeof(string_t))
			{
				*static_cast<string_t*>(get_reflected_field_ptr(object, field)) = value != nullptr ? value : "";
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

		span_t<const reflected_enum_value_desc_t> get_reflected_enum_values(const reflected_field_desc_t& field);

		i64 read_reflected_enum(const void* object, const reflected_field_desc_t& field)
		{
			u64 raw = 0;
			if (field.get != nullptr)
			{
				if (field.type == reflected_value_type_e::enum8)
				{
					u8 value = 0;
					field.get(object, field, &value, field.user_data);
					raw = value;
				}
				else
				{
					u32 value = 0;
					field.get(object, field, &value, field.user_data);
					raw = value;
				}
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

			const span_t<const reflected_enum_value_desc_t> enum_values = get_reflected_enum_values(field);
			for (u32 i = 0; i < enum_values.size; ++i)
			{
				const i64 value = enum_values.data[i].value;
				if (value == static_cast<i64>(raw) || value == static_cast<i64>(static_cast<i32>(raw)))
					return value;
			}
			return static_cast<i64>(raw);
		}

		span_t<const reflected_enum_value_desc_t> get_reflected_enum_values(const reflected_field_desc_t& field)
		{
			if (field.enum_values.size != 0)
				return field.enum_values;

			sid_t type_id = field.sub_type_id;
			if (type_id == 0 || reflected_value_type_from_sub_type_id(type_id) != reflected_value_type_e::invalid)
				type_id = field.value_type_id;
			if (type_id == 0)
				return {};

			const reflected_type_desc_t* type = reflection_registry_t::get().find_type(type_id);
			return type != nullptr ? type->enum_values : span_t<const reflected_enum_value_desc_t>{};
		}

		void write_reflected_enum(void* object, const reflected_field_desc_t& field, i64 value)
		{
			if ((field.flags & reflected_field_flags_read_only) != 0)
				return;
			if (field.set != nullptr)
			{
				if (field.type == reflected_value_type_e::enum8)
				{
					const u8 raw = static_cast<u8>(value);
					field.set(object, field, &raw, field.user_data);
				}
				else
				{
					const u32 raw = static_cast<u32>(value);
					field.set(object, field, &raw, field.user_data);
				}
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

		sid_t get_object_type_id(const reflected_field_desc_t& field)
		{
			return field.value_type_id != 0 ? field.value_type_id : field.sub_type_id;
		}

		reflected_field_desc_t get_vector_item_field(const reflected_field_desc_t& field)
		{
			const reflected_value_type_e type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (type != reflected_value_type_e::invalid)
			{
				return {
					.name		   = "value",
					.display_name  = "Value",
					.type		   = type,
					.value_type_id = field.value_type_id,
					.size		   = reflected_value_type_size(type),
					.min		   = field.min,
					.max		   = field.max,
					.flags		   = field.flags & (reflected_field_flags_read_only | reflected_field_flags_clamped),
				};
			}

			const reflected_type_desc_t* sub_type = reflection_registry_t::get().find_type(field.sub_type_id);
			if (sub_type != nullptr && sub_type->fields.size == 0 && sub_type->enum_values.size != 0)
			{
				return {
					.name		   = "value",
					.display_name  = "Value",
					.type		   = sub_type->size == sizeof(u8) ? reflected_value_type_e::enum8 : reflected_value_type_e::enum32,
					.value_type_id = sub_type->type_id,
					.sub_type_id   = sub_type->type_id,
					.size		   = sub_type->size,
					.min		   = field.min,
					.max		   = field.max,
					.flags		   = field.flags & (reflected_field_flags_read_only | reflected_field_flags_clamped),
				};
			}

			return {
				.name		   = "value",
				.display_name  = "Value",
				.type		   = sub_type != nullptr ? reflected_value_type_e::object : reflected_value_type_e::invalid,
				.value_type_id = sub_type != nullptr ? sub_type->type_id : 0,
				.size		   = sub_type != nullptr ? sub_type->size : 0,
				.min		   = field.min,
				.max		   = field.max,
				.flags		   = field.flags & (reflected_field_flags_read_only | reflected_field_flags_clamped),
			};
		}

		bool is_reflected_container_ops_valid(const reflected_container_ops_t& ops)
		{
			return ops.get_count != nullptr && ops.get_item != nullptr && ops.get_const_item != nullptr && ops.clear != nullptr && ops.resize != nullptr && ops.add != nullptr && ops.remove != nullptr;
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

		template <typename T> T* get_reflected_container_data(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::static_vector)
				return get_reflected_static_vector_data<T>(object, field);
			return get_reflected_vector<T>(object, field).data();
		}

		template <typename T> u32 get_reflected_container_item_count(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::static_vector)
				return static_cast<u32>(get_reflected_static_vector_size<T>(object, field));
			return static_cast<u32>(get_reflected_vector<T>(object, field).size());
		}

		template <typename T> void clear_reflected_container(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::static_vector)
				clear_reflected_static_vector<T>(object, field);
			else
				get_reflected_vector<T>(object, field).resize(0);
		}

		template <typename T> void add_reflected_container_item(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::static_vector)
				add_reflected_static_vector_item<T>(object, field);
			else
				get_reflected_vector<T>(object, field).push_back(T{});
		}

		template <typename T> void remove_reflected_container_item(void* object, const reflected_field_desc_t& field, u32 index)
		{
			if (field.type == reflected_value_type_e::static_vector)
			{
				remove_reflected_static_vector_item<T>(object, field, index);
			}
			else
			{
				vector_t<T>& values = get_reflected_vector<T>(object, field);
				values.erase(values.begin() + index);
			}
		}
	}

	void editor_widget_reflect_type_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reflect_type");
		tree.attach(parent, _root);

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
			if ((field.flags & reflected_field_flags_no_ui) != 0)
				continue;
			if (is_vector_field(field) && is_vector_unfolded(field.id))
				control_count += get_vector_item_count(field);
		}

		_controls.reserve(control_count);
		_vector_controls.reserve(type->fields.size);
		_vector_item_controls.reserve(control_count);
		_vector_item_fields.reserve(control_count);
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if ((field.flags & reflected_field_flags_no_ui) != 0)
				continue;
			const char* label = field.display_name != nullptr ? field.display_name : field.name;
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
		case reflected_value_type_e::quat: {
			editor_vec4_field_t*	   vec	  = new editor_vec4_field_t();
			editor_vec4_field_config_t config = {};
			const quat_t			   value  = read_reflected_value<quat_t>(object, field);
			config.value					  = {value.x, value.y, value.z, value.w};
			config.on_changed				  = on_vec4_changed;
			config.user_data				  = control;
			vec->init(*_ui, parent, config);
			center_property_row_control(*_ui, vec->get_root());
			_vec4_fields.push_back(vec);
			break;
		}
		case reflected_value_type_e::object: {
			const sid_t type_id = get_object_type_id(field);
			if (type_id == type_id_t<vec2f_t>::value)
			{
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
			if (type_id == type_id_t<vec2u_t>::value)
			{
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				config.value					  = to_vec2f(read_reflected_value<vec2u_t>(object, field));
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				center_property_row_control(*_ui, vec->get_root());
				_vec2_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec2u16_t>::value)
			{
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				config.value					  = to_vec2f(read_reflected_value<vec2u16_t>(object, field));
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				center_property_row_control(*_ui, vec->get_root());
				_vec2_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec3f_t>::value)
			{
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
			if (type_id == type_id_t<vec3u_t>::value)
			{
				editor_vec3_field_t*	   vec	  = new editor_vec3_field_t();
				editor_vec3_field_config_t config = {};
				config.value					  = to_vec3f(read_reflected_value<vec3u_t>(object, field));
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec3_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				center_property_row_control(*_ui, vec->get_root());
				_vec3_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec4f_t>::value)
			{
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
			if (type_id == type_id_t<vec4u_t>::value)
			{
				editor_vec4_field_t*	   vec	  = new editor_vec4_field_t();
				editor_vec4_field_config_t config = {};
				config.value					  = to_vec4f(read_reflected_value<vec4u_t>(object, field));
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec4_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				center_property_row_control(*_ui, vec->get_root());
				_vec4_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<color_t>::value)
			{
				editor_color_field_t*		color  = new editor_color_field_t();
				editor_color_field_config_t config = {};
				config.color					   = read_reflected_value<color_t>(object, field).to_vector();
				config.on_changed				   = on_color_changed;
				config.user_data				   = control;
				color->init(*_ui, parent, config);
				center_property_row_control(*_ui, color->get_root());
				_color_fields.push_back(color);
				break;
			}

			add_unknown_label(*_ui, parent);
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
		case reflected_value_type_e::enum8:
		case reflected_value_type_e::enum32: {
			const span_t<const reflected_enum_value_desc_t> enum_values = get_reflected_enum_values(field);
			if (enum_values.size == 0)
			{
				add_unknown_label(*_ui, parent);
				break;
			}

			_dropdowns.push_back({});
			enum_control_t& enum_control = _dropdowns.back();
			enum_control.items.reserve(enum_values.size);
			for (u32 enum_index = 0; enum_index < enum_values.size; ++enum_index)
			{
				const reflected_enum_value_desc_t& value = enum_values.data[enum_index];
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

		if (is_reflected_container_ops_valid(field.container_ops))
		{
			const reflected_field_desc_t item_field = get_vector_item_field(field);
			if (item_field.type == reflected_value_type_e::invalid)
				return;

			const u32 item_count = field.container_ops.get_count(_object, field);
			for (u32 i = 0; i < item_count; ++i)
			{
				char item_label[32] = {};
				std::snprintf(item_label, sizeof(item_label), "[%u]", i);
				const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, item_label, true, true);
				_rows.push_back(row);
				_vector_item_fields.push_back(item_field);
				_vector_item_controls.push_back({.owner = this, .field = &field, .index = i});
				ui::listener_bundle_t remove_listener = {};
				remove_listener.user_data			  = &_vector_item_controls.back();
				remove_listener.on_click			  = on_vector_item_remove_click;
				_ui->get_input().set_listener(row.remove_button, remove_listener);
				install_reflected_control(row.right, _vector_item_fields.back(), field.container_ops.get_item(_object, field, i));
			}
			return;
		}

		const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
		const reflected_field_desc_t item_field = get_vector_item_field(field);

		auto install_items = [&]<typename T>() {
			const u32 item_count = get_reflected_container_item_count<T>(_object, field);
			T*		  values	 = get_reflected_container_data<T>(_object, field);
			for (u32 i = 0; i < item_count; ++i)
			{
				char item_label[32] = {};
				std::snprintf(item_label, sizeof(item_label), "[%u]", i);
				const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, item_label, true, true);
				_rows.push_back(row);
				_vector_item_fields.push_back(item_field);
				_vector_item_controls.push_back({.owner = this, .field = &field, .index = i});
				ui::listener_bundle_t remove_listener = {};
				remove_listener.user_data			  = &_vector_item_controls.back();
				remove_listener.on_click			  = on_vector_item_remove_click;
				_ui->get_input().set_listener(row.remove_button, remove_listener);
				install_reflected_control(row.right, _vector_item_fields.back(), &values[i]);
			}
		};

		switch (value_type)
		{
		case reflected_value_type_e::f32:
			install_items.template operator()<f32>();
			break;
		case reflected_value_type_e::i32:
			install_items.template operator()<i32>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::entity_id:
		case reflected_value_type_e::enum32:
			install_items.template operator()<u32>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			install_items.template operator()<u8>();
			break;
		case reflected_value_type_e::resource:
			install_items.template operator()<sid_t>();
			break;
		case reflected_value_type_e::string:
			install_items.template operator()<string_t>();
			break;
		case reflected_value_type_e::json:
			install_items.template operator()<nlohmann::json>();
			break;
		case reflected_value_type_e::quat:
			install_items.template operator()<quat_t>();
			break;
		default:
			break;
		}
	}

	u32 editor_widget_reflect_type_t::get_vector_item_count(const reflected_field_desc_t& field) const
	{
		if (is_reflected_container_ops_valid(field.container_ops))
			return field.container_ops.get_count(_object, field);

		auto get_count = [&]<typename T>() { return get_reflected_container_item_count<T>(_object, field); };

		switch (reflected_value_type_from_sub_type_id(field.sub_type_id))
		{
		case reflected_value_type_e::f32:
			return get_count.template operator()<f32>();
		case reflected_value_type_e::i32:
			return get_count.template operator()<i32>();
		case reflected_value_type_e::u32:
		case reflected_value_type_e::entity_id:
		case reflected_value_type_e::enum32:
			return get_count.template operator()<u32>();
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			return get_count.template operator()<u8>();
		case reflected_value_type_e::resource:
			return get_count.template operator()<sid_t>();
		case reflected_value_type_e::string:
			return get_count.template operator()<string_t>();
		case reflected_value_type_e::json:
			return get_count.template operator()<nlohmann::json>();
		case reflected_value_type_e::quat:
			return get_count.template operator()<quat_t>();
		default:
			return 0;
		}
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

		if (is_reflected_container_ops_valid(field.container_ops))
		{
			field.container_ops.clear(_object, field);
			rebuild_reflected_controls();
			return;
		}

		auto clear_items = [&]<typename T>() {
			clear_reflected_container<T>(_object, field);
			rebuild_reflected_controls();
		};

		switch (reflected_value_type_from_sub_type_id(field.sub_type_id))
		{
		case reflected_value_type_e::f32:
			clear_items.template operator()<f32>();
			break;
		case reflected_value_type_e::i32:
			clear_items.template operator()<i32>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::entity_id:
		case reflected_value_type_e::enum32:
			clear_items.template operator()<u32>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			clear_items.template operator()<u8>();
			break;
		case reflected_value_type_e::resource:
			clear_items.template operator()<sid_t>();
			break;
		case reflected_value_type_e::string:
			clear_items.template operator()<string_t>();
			break;
		case reflected_value_type_e::json:
			clear_items.template operator()<nlohmann::json>();
			break;
		case reflected_value_type_e::quat:
			clear_items.template operator()<quat_t>();
			break;
		default:
			break;
		}
	}

	void editor_widget_reflect_type_t::add_vector_item(const reflected_field_desc_t& field)
	{
		if ((field.flags & reflected_field_flags_read_only) != 0)
			return;

		if (is_reflected_container_ops_valid(field.container_ops))
		{
			field.container_ops.add(_object, field);
			rebuild_reflected_controls();
			return;
		}

		auto add_item = [&]<typename T>() {
			add_reflected_container_item<T>(_object, field);
			rebuild_reflected_controls();
		};

		switch (reflected_value_type_from_sub_type_id(field.sub_type_id))
		{
		case reflected_value_type_e::f32:
			add_item.template operator()<f32>();
			break;
		case reflected_value_type_e::i32:
			add_item.template operator()<i32>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::entity_id:
		case reflected_value_type_e::enum32:
			add_item.template operator()<u32>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			add_item.template operator()<u8>();
			break;
		case reflected_value_type_e::resource:
			add_item.template operator()<sid_t>();
			break;
		case reflected_value_type_e::string:
			add_item.template operator()<string_t>();
			break;
		case reflected_value_type_e::json:
			add_item.template operator()<nlohmann::json>();
			break;
		case reflected_value_type_e::quat:
			add_item.template operator()<quat_t>();
			break;
		default:
			break;
		}
	}

	void editor_widget_reflect_type_t::remove_vector_item(const reflected_field_desc_t& field, u32 index)
	{
		if ((field.flags & reflected_field_flags_read_only) != 0)
			return;

		if (is_reflected_container_ops_valid(field.container_ops))
		{
			field.container_ops.remove(_object, field, index);
			rebuild_reflected_controls();
			return;
		}

		auto remove_item = [&]<typename T>() {
			remove_reflected_container_item<T>(_object, field, index);
			rebuild_reflected_controls();
		};

		switch (reflected_value_type_from_sub_type_id(field.sub_type_id))
		{
		case reflected_value_type_e::f32:
			remove_item.template operator()<f32>();
			break;
		case reflected_value_type_e::i32:
			remove_item.template operator()<i32>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::entity_id:
		case reflected_value_type_e::enum32:
			remove_item.template operator()<u32>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			remove_item.template operator()<u8>();
			break;
		case reflected_value_type_e::resource:
			remove_item.template operator()<sid_t>();
			break;
		case reflected_value_type_e::string:
			remove_item.template operator()<string_t>();
			break;
		case reflected_value_type_e::json:
			remove_item.template operator()<nlohmann::json>();
			break;
		case reflected_value_type_e::quat:
			remove_item.template operator()<quat_t>();
			break;
		default:
			break;
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
		if (get_object_type_id(*control.field) == type_id_t<color_t>::value)
			write_reflected_value(control.object, *control.field, color_t{value.x, value.y, value.z, value.w});
	}

	void editor_widget_reflect_type_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		const sid_t			 type_id = get_object_type_id(*control.field);
		if (type_id == type_id_t<vec2u_t>::value)
			write_reflected_value(control.object, *control.field, to_vec2u(value));
		else if (type_id == type_id_t<vec2u16_t>::value)
			write_reflected_value(control.object, *control.field, to_vec2u16(value));
		else
			write_reflected_value(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		if (get_object_type_id(*control.field) == type_id_t<vec3u_t>::value)
			write_reflected_value(control.object, *control.field, to_vec3u(value));
		else
			write_reflected_value(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		const sid_t			 type_id = get_object_type_id(*control.field);
		if (type_id == type_id_t<vec4u_t>::value)
			write_reflected_value(control.object, *control.field, to_vec4u(value));
		else if (control.field->type == reflected_value_type_e::quat)
			write_reflected_value(control.object, *control.field, quat_t{value.x, value.y, value.z, value.w});
		else
			write_reflected_value(control.object, *control.field, value);
	}

	u16 editor_widget_reflect_type_t::on_enum_selected(void* user_data)
	{
		reflected_control_t&							control		= *static_cast<reflected_control_t*>(user_data);
		const i64										value		= read_reflected_enum(control.object, *control.field);
		const span_t<const reflected_enum_value_desc_t> enum_values = get_reflected_enum_values(*control.field);
		for (u16 i = 0; i < enum_values.size; ++i)
		{
			if (enum_values.data[i].value == value)
				return i;
		}
		return 0;
	}

	void editor_widget_reflect_type_t::on_enum_pressed(u16 value, void* user_data)
	{
		reflected_control_t&							control		= *static_cast<reflected_control_t*>(user_data);
		const span_t<const reflected_enum_value_desc_t> enum_values = get_reflected_enum_values(*control.field);
		SFG_ASSERT(value < enum_values.size);
		write_reflected_enum(control.object, *control.field, enum_values.data[value].value);
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
		_vector_item_fields.resize(0);
	}
}
