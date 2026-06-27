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
#include "assets/editor_asset.hpp"
#include "editor_app.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/ostream.hpp>
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
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>

namespace sfg
{
#define EDITOR_REFLECTED_RESOURCE_HANDLE_CASES                                                                                                                                                                                                                     \
	case reflected_value_type_e::audio_handle:                                                                                                                                                                                                                     \
	case reflected_value_type_e::font_handle:                                                                                                                                                                                                                      \
	case reflected_value_type_e::mesh_handle:                                                                                                                                                                                                                      \
	case reflected_value_type_e::skeleton_handle:                                                                                                                                                                                                                  \
	case reflected_value_type_e::animation_handle:                                                                                                                                                                                                                 \
	case reflected_value_type_e::material_handle:                                                                                                                                                                                                                  \
	case reflected_value_type_e::shader_handle:                                                                                                                                                                                                                    \
	case reflected_value_type_e::texture_handle:                                                                                                                                                                                                                   \
	case reflected_value_type_e::texture_sampler_handle:                                                                                                                                                                                                           \
	case reflected_value_type_e::physical_material_handle:                                                                                                                                                                                                         \
	case reflected_value_type_e::prefab_handle:                                                                                                                                                                                                                    \
	case reflected_value_type_e::animation_state_machine_handle:                                                                                                                                                                                                   \
	case reflected_value_type_e::hdr_skybox_handle:

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
			config.type		 = field.flags.is_set(reflected_field_flags_clamped) ? editor_input_field_type_e::number_slider : editor_input_field_type_e::number;
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

		void set_field_tooltip(ui::ui_context& ui, ui::widget_id_t owner, const reflected_field_desc_t& field)
		{
			if (field.tooltip == nullptr || field.tooltip[0] == '\0')
				return;

			editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(ui);
			if (tooltip_controller == nullptr)
				return;

			editor_tooltip_desc_t tooltip = {};
			tooltip.text				  = field.tooltip;
			tooltip_controller->set_tooltip(owner, tooltip);
		}

		template <typename T> T read_reflected_value(const void* object, const reflected_field_desc_t& field)
		{
			return *reinterpret_cast<const T*>(static_cast<const u8*>(object) + field.offset);
		}

		template <typename T> void write_reflected_value(void* object, const reflected_field_desc_t& field, const T& value)
		{
			if (field.flags.is_set(reflected_field_flags_read_only))
				return;
			*reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset) = value;
		}

		f32 read_reflected_number(const void* object, const reflected_field_desc_t& field)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32:
				return read_reflected_value<f32>(object, field);
			case reflected_value_type_e::i32:
				return static_cast<f32>(read_reflected_value<i32>(object, field));
			case reflected_value_type_e::i16:
				return static_cast<f32>(read_reflected_value<i16>(object, field));
			case reflected_value_type_e::i8:
				return static_cast<f32>(read_reflected_value<i8>(object, field));
			case reflected_value_type_e::u32:
				return static_cast<f32>(read_reflected_value<u32>(object, field));
			case reflected_value_type_e::u64:
				return static_cast<f32>(read_reflected_value<u64>(object, field));
			case reflected_value_type_e::u16:
				return static_cast<f32>(read_reflected_value<u16>(object, field));
			case reflected_value_type_e::size_t:
				return static_cast<f32>(read_reflected_value<size_t>(object, field));
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
			case reflected_value_type_e::i16:
				write_reflected_value(object, field, static_cast<i16>(value < -32768.0f ? -32768.0f : (value > 32767.0f ? 32767.0f : value)));
				break;
			case reflected_value_type_e::i8:
				write_reflected_value(object, field, static_cast<i8>(value < -128.0f ? -128.0f : (value > 127.0f ? 127.0f : value)));
				break;
			case reflected_value_type_e::u32:
				write_reflected_value(object, field, static_cast<u32>(value < 0.0f ? 0.0f : value));
				break;
			case reflected_value_type_e::u64:
				write_reflected_value(object, field, static_cast<u64>(value < 0.0f ? 0.0f : value));
				break;
			case reflected_value_type_e::u16:
				write_reflected_value(object, field, static_cast<u16>(value < 0.0f ? 0.0f : (value > 65535.0f ? 65535.0f : value)));
				break;
			case reflected_value_type_e::size_t:
				write_reflected_value(object, field, static_cast<size_t>(value < 0.0f ? 0.0f : value));
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
			return *(static_cast<const u8*>(object) + field.offset) != 0;
		}

		void write_reflected_bool(void* object, const reflected_field_desc_t& field, bool value)
		{
			if (field.flags.is_set(reflected_field_flags_read_only))
				return;
			*(static_cast<u8*>(object) + field.offset) = value ? 1 : 0;
		}

		const char* read_reflected_text(const void* object, const reflected_field_desc_t& field)
		{
			if (field.size == sizeof(string_t))
				return reinterpret_cast<const string_t*>(static_cast<const u8*>(object) + field.offset)->c_str();
			return reinterpret_cast<const char*>(static_cast<const u8*>(object) + field.offset);
		}

		void write_reflected_text(void* object, const reflected_field_desc_t& field, const char* value)
		{
			if (field.flags.is_set(reflected_field_flags_read_only))
				return;

			if (field.size == sizeof(string_t))
			{
				*reinterpret_cast<string_t*>(static_cast<u8*>(object) + field.offset) = value != nullptr ? value : "";
				return;
			}

			SFG_ASSERT(field.size > 0);
			char*		 dst	 = reinterpret_cast<char*>(static_cast<u8*>(object) + field.offset);
			const char*	 src	 = value != nullptr ? value : "";
			const size_t max_len = static_cast<size_t>(field.size - 1);
			const size_t len	 = std::strlen(src) < max_len ? std::strlen(src) : max_len;
			std::memcpy(dst, src, len);
			dst[len] = '\0';
		}

		span_t<const reflected_enum_value_desc_t> get_reflected_enum_values(const reflected_field_desc_t& field);

		i64 read_reflected_enum(const void* object, const reflected_field_desc_t& field)
		{
			u64			raw = 0;
			const void* ptr = static_cast<const u8*>(object) + field.offset;
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
			if (field.flags.is_set(reflected_field_flags_read_only))
				return;

			void* ptr = static_cast<u8*>(object) + field.offset;
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

		bitmask32 get_vector_item_field_flags(const reflected_field_desc_t& field)
		{
			bitmask32 flags = reflected_field_flags_none;
			flags.set(reflected_field_flags_read_only, field.flags.is_set(reflected_field_flags_read_only));
			flags.set(reflected_field_flags_clamped, field.flags.is_set(reflected_field_flags_clamped));
			return flags;
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
					.flags		   = get_vector_item_field_flags(field),
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
					.flags		   = get_vector_item_field_flags(field),
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
				.flags		   = get_vector_item_field_flags(field),
			};
		}

		template <typename T> size_t get_inplace_vector_head_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = sizeof(T) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		template <typename T> void clear_reflected_inplace_vector(void* object, const reflected_field_desc_t& field)
		{
			T*		data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			size_t& size = *reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			while (size > 0)
			{
				--size;
				std::destroy_at(data + size);
			}
		}

		template <typename T> void add_reflected_inplace_vector_item(void* object, const reflected_field_desc_t& field)
		{
			T*		data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			size_t& size = *reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			if (size == field.capacity)
				return;
			std::construct_at(data + size);
			++size;
		}

		template <typename T> void remove_reflected_inplace_vector_item(void* object, const reflected_field_desc_t& field, u32 index)
		{
			T*		data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			size_t& size = *reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			SFG_ASSERT(index < size);
			for (size_t i = index; i < size - 1; ++i)
				data[i] = std::move(data[i + 1]);
			--size;
			std::destroy_at(data + size);
		}

		template <typename T> T* get_reflected_container_data(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::inplace_vector)
				return std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			return reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset)->data();
		}

		template <typename T> u32 get_reflected_container_item_count(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::inplace_vector)
				return static_cast<u32>(*reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field)));
			return static_cast<u32>(reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset)->size());
		}

		template <typename T> void clear_reflected_container(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::inplace_vector)
				clear_reflected_inplace_vector<T>(object, field);
			else
				reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset)->resize(0);
		}

		template <typename T> void add_reflected_container_item(void* object, const reflected_field_desc_t& field)
		{
			if (field.type == reflected_value_type_e::inplace_vector)
				add_reflected_inplace_vector_item<T>(object, field);
			else
				reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset)->push_back(T{});
		}

		template <typename T> void remove_reflected_container_item(void* object, const reflected_field_desc_t& field, u32 index)
		{
			if (field.type == reflected_value_type_e::inplace_vector)
			{
				remove_reflected_inplace_vector_item<T>(object, field, index);
			}
			else
			{
				vector_t<T>& values = *reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset);
				values.erase(values.begin() + index);
			}
		}
	}

	void editor_widget_reflect_type_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui				  = &ui;
		_command_listener = editor_app_t::get().get_command_system().add_listener(on_command_system_changed, this);

		ui::layout_tree_t& tree	 = ui.get_tree();
		editor_theme_t&	   theme = editor_theme_t::get();

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
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
	}

	void editor_widget_reflect_type_t::uninit()
	{
		clear_reflected_controls();
		editor_app_t::get().get_command_system().remove_listener(_command_listener);
		_ui->deallocate_widget(_root);

		_ui				  = nullptr;
		_object			  = nullptr;
		_type_id		  = 0;
		_root			  = NULL_WIDGET;
		_target			  = {};
		_command_listener = {};
		_vector_states.resize(0);
	}

	void editor_widget_reflect_type_t::set_reflected_obj(void* object, sid_t type_id)
	{
		editor_reflected_edit_target_t target = {};
		target.object						  = object;
		target.required_listener			  = _command_listener;
		target.type_id						  = type_id;
		target.kind							  = editor_reflected_edit_target_kind_e::raw_object;
		set_reflected_obj(object, type_id, target);
	}

	void editor_widget_reflect_type_t::set_reflected_obj(void* object, sid_t type_id, const editor_reflected_edit_target_t& target)
	{
		const bool same_target = _object == object && _type_id == type_id && _target.object == target.object && _target.entities == target.entities && _target.world == target.world && _target.entity == target.entity && _target.type_id == target.type_id &&
								 _target.entity_count == target.entity_count && _target.kind == target.kind;

		_object	 = object;
		_type_id = type_id;
		_target	 = target;
		if (!same_target)
			_vector_states.resize(0);
		rebuild_reflected_controls();
	}

	void editor_widget_reflect_type_t::set_vector_fold_states(const vector_t<vector_fold_state_t>& states)
	{
		_vector_states = states;
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
			if (field.flags.is_set(reflected_field_flags_no_ui))
				continue;
			if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && is_vector_unfolded(field.id))
				control_count += get_vector_item_count(field);
		}

		_controls.reserve(control_count);
		_vector_controls.reserve(type->fields.size);
		_vector_item_controls.reserve(control_count);
		_vector_item_fields.reserve(control_count);
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if (field.flags.is_set(reflected_field_flags_no_ui))
				continue;
			const char* label = field.display_name != nullptr ? field.display_name : field.name;
			if (field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector)
			{
				install_vector_field(field, label);
				continue;
			}

			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label);
			set_field_tooltip(*_ui, row.label, field);
			_rows.push_back(row);
			install_reflected_control(row.right, field, _object, field, _object);
		}
	}

	void editor_widget_reflect_type_t::install_reflected_control(ui::widget_id_t parent, const reflected_field_desc_t& field, void* object, const reflected_field_desc_t& command_field, void* command_object)
	{
		_controls.push_back({.owner = this, .field = &field, .command_field = &command_field, .object = object, .command_object = command_object, .mixed = is_field_mixed(command_field, command_object)});
		reflected_control_t* control = &_controls.back();

		if (editor_asset_util_t::reflected_value_type_to_asset_type(field.type) != editor_asset_type_e::invalid)
		{
			editor_widget_asset_reference_t*	   reference = new editor_widget_asset_reference_t();
			editor_widget_asset_reference_config_t config	 = {};
			config.asset_type								 = editor_asset_util_t::reflected_value_type_to_asset_type(field.type);
			config.selected									 = on_asset_selected;
			config.pressed									 = on_asset_pressed;
			config.user_data								 = control;
			reference->init(*_ui, parent, config);
			reference->set_mixed(control->mixed);
			center_property_row_control(*_ui, reference->get_root());
			control->widget		 = reference;
			control->widget_type = reflected_control_widget_e::asset_reference;
			_asset_references.push_back(reference);
			return;
		}

		if (field.type == reflected_value_type_e::entity_guid)
		{
			editor_widget_entity_guid_reference_t*		 reference = new editor_widget_entity_guid_reference_t();
			editor_widget_entity_guid_reference_config_t config	   = {};
			config.world										   = _target.world.is_null() ? editor_app_t::get().get_main_world() : _target.world;
			config.selected										   = on_entity_selected;
			config.pressed										   = on_entity_pressed;
			config.user_data									   = control;
			reference->init(*_ui, parent, config);
			reference->set_mixed(control->mixed);
			center_property_row_control(*_ui, reference->get_root());
			control->widget		 = reference;
			control->widget_type = reflected_control_widget_e::entity_guid_reference;
			_entity_references.push_back(reference);
			return;
		}

		if (field.type == reflected_value_type_e::text_id && command_field.type == reflected_value_type_e::text_id)
		{
			editor_widget_text_id_t*	   text_id = new editor_widget_text_id_t();
			editor_widget_text_id_config_t config  = {};
			config.world						   = _target.world.is_null() ? editor_app_t::get().get_main_world() : _target.world;
			config.selected						   = on_text_id_selected;
			config.submitted					   = on_text_id_submitted;
			config.user_data					   = control;
			text_id->init(*_ui, parent, config);
			text_id->set_mixed(control->mixed);
			center_property_row_control(*_ui, text_id->get_root());
			control->widget		 = text_id;
			control->widget_type = reflected_control_widget_e::text_id;
			_text_ids.push_back(text_id);
			return;
		}

		switch (field.type)
		{
		case reflected_value_type_e::f32: {
			editor_input_field_t*		input  = new editor_input_field_t();
			editor_input_field_config_t config = {};
			apply_reflected_number_config(config, field, false);
			config.placeholder		 = control->mixed ? "Mixed" : nullptr;
			config.number_value		 = read_reflected_number(object, field);
			config.on_number_changed = on_number_changed;
			config.on_submitted		 = on_input_submitted;
			config.user_data		 = control;
			input->init(*_ui, parent, config);
			if (control->mixed)
				input->set_text("");
			center_property_row_control(*_ui, input->get_root());
			control->widget		 = input;
			control->widget_type = reflected_control_widget_e::input;
			_input_fields.push_back(input);
			break;
		}
		case reflected_value_type_e::i32:
		case reflected_value_type_e::i16:
		case reflected_value_type_e::i8:
		case reflected_value_type_e::u32:
		case reflected_value_type_e::u64:
		case reflected_value_type_e::u16:
		case reflected_value_type_e::size_t:
		case reflected_value_type_e::u8: {
			editor_input_field_t*		input  = new editor_input_field_t();
			editor_input_field_config_t config = {};
			apply_reflected_number_config(config, field, true);
			config.placeholder		 = control->mixed ? "Mixed" : nullptr;
			config.number_value		 = read_reflected_number(object, field);
			config.on_number_changed = on_number_changed;
			config.on_submitted		 = on_input_submitted;
			config.user_data		 = control;
			input->init(*_ui, parent, config);
			if (control->mixed)
				input->set_text("");
			center_property_row_control(*_ui, input->get_root());
			control->widget		 = input;
			control->widget_type = reflected_control_widget_e::input;
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
			checkbox->set_mixed(control->mixed);
			center_property_row_control(*_ui, checkbox->get_root());
			control->widget		 = checkbox;
			control->widget_type = reflected_control_widget_e::checkbox;
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
			vec->set_mixed(control->mixed);
			center_property_row_control(*_ui, vec->get_root());
			control->widget		 = vec;
			control->widget_type = reflected_control_widget_e::vec4;
			_vec4_fields.push_back(vec);
			break;
		}
		case reflected_value_type_e::object: {
			const sid_t type_id = field.value_type_id != 0 ? field.value_type_id : field.sub_type_id;
			if (type_id == type_id_t<vec2f_t>::value)
			{
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				config.value					  = read_reflected_value<vec2f_t>(object, field);
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec2;
				_vec2_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec2u_t>::value)
			{
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				const vec2u_t			   value  = read_reflected_value<vec2u_t>(object, field);
				config.value					  = {static_cast<f32>(value.x), static_cast<f32>(value.y)};
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec2;
				_vec2_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec2u16_t>::value)
			{
				editor_vec2_field_t*	   vec	  = new editor_vec2_field_t();
				editor_vec2_field_config_t config = {};
				const vec2u16_t			   value  = read_reflected_value<vec2u16_t>(object, field);
				config.value					  = {static_cast<f32>(value.x), static_cast<f32>(value.y)};
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec2_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec2;
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
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec3;
				_vec3_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec3u_t>::value)
			{
				editor_vec3_field_t*	   vec	  = new editor_vec3_field_t();
				editor_vec3_field_config_t config = {};
				const vec3u_t			   value  = read_reflected_value<vec3u_t>(object, field);
				config.value					  = {static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z)};
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec3_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec3;
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
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec4;
				_vec4_fields.push_back(vec);
				break;
			}
			if (type_id == type_id_t<vec4u_t>::value)
			{
				editor_vec4_field_t*	   vec	  = new editor_vec4_field_t();
				editor_vec4_field_config_t config = {};
				const vec4u_t			   value  = read_reflected_value<vec4u_t>(object, field);
				config.value					  = {static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z), static_cast<f32>(value.w)};
				config.increment				  = 1.0f;
				config.integer					  = true;
				config.on_changed				  = on_vec4_changed;
				config.user_data				  = control;
				vec->init(*_ui, parent, config);
				vec->set_mixed(control->mixed);
				center_property_row_control(*_ui, vec->get_root());
				control->widget		 = vec;
				control->widget_type = reflected_control_widget_e::vec4;
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
				control->widget		 = color;
				control->widget_type = reflected_control_widget_e::color;
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
			config.placeholder				   = control->mixed ? "Mixed" : nullptr;
			config.text_value				   = control->mixed ? "" : read_reflected_text(object, field);
			config.on_text_changed			   = on_text_changed;
			config.on_submitted				   = on_input_submitted;
			config.user_data				   = control;
			input->init(*_ui, parent, config);
			center_property_row_control(*_ui, input->get_root());
			control->widget		 = input;
			control->widget_type = reflected_control_widget_e::input;
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
			config.title					= control->mixed ? "Mixed" : nullptr;
			config.title_from_selection		= !control->mixed;
			config.selected					= on_enum_selected;
			config.pressed					= on_enum_pressed;
			config.user_data				= control;
			enum_control.dropdown->init(*_ui, parent, config);
			center_property_row_control(*_ui, enum_control.dropdown->get_root());
			control->widget		 = enum_control.dropdown;
			control->widget_type = reflected_control_widget_e::dropdown;
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
		set_field_tooltip(*_ui, vector_row.label, field);
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
				set_field_tooltip(*_ui, row.label, field);
				_rows.push_back(row);
				_vector_item_fields.push_back(item_field);
				_vector_item_controls.push_back({.owner = this, .field = &field, .index = i});
				ui::listener_bundle_t remove_listener = {};
				remove_listener.user_data			  = &_vector_item_controls.back();
				remove_listener.on_click			  = on_vector_item_remove_click;
				_ui->get_input().set_listener(row.remove_button, remove_listener);
				install_reflected_control(row.right, _vector_item_fields.back(), &values[i], field, _object);
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
		case reflected_value_type_e::i16:
			install_items.template operator()<i16>();
			break;
		case reflected_value_type_e::i8:
			install_items.template operator()<i8>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::enum32:
			install_items.template operator()<u32>();
			break;
		case reflected_value_type_e::u64:
			install_items.template operator()<u64>();
			break;
		case reflected_value_type_e::u16:
			install_items.template operator()<u16>();
			break;
		case reflected_value_type_e::size_t:
			install_items.template operator()<size_t>();
			break;
		case reflected_value_type_e::entity_guid:
			install_items.template operator()<entity_guid_t>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			install_items.template operator()<u8>();
			break;
			EDITOR_REFLECTED_RESOURCE_HANDLE_CASES
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
		auto get_count = [&]<typename T>() { return get_reflected_container_item_count<T>(_object, field); };

		switch (reflected_value_type_from_sub_type_id(field.sub_type_id))
		{
		case reflected_value_type_e::f32:
			return get_count.template operator()<f32>();
		case reflected_value_type_e::i32:
			return get_count.template operator()<i32>();
		case reflected_value_type_e::i16:
			return get_count.template operator()<i16>();
		case reflected_value_type_e::i8:
			return get_count.template operator()<i8>();
		case reflected_value_type_e::u32:
		case reflected_value_type_e::enum32:
			return get_count.template operator()<u32>();
		case reflected_value_type_e::u64:
			return get_count.template operator()<u64>();
		case reflected_value_type_e::u16:
			return get_count.template operator()<u16>();
		case reflected_value_type_e::size_t:
			return get_count.template operator()<size_t>();
		case reflected_value_type_e::entity_guid:
			return get_count.template operator()<entity_guid_t>();
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			return get_count.template operator()<u8>();
			EDITOR_REFLECTED_RESOURCE_HANDLE_CASES
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
		if (field.flags.is_set(reflected_field_flags_read_only))
			return;

		ostream_t  old_value;
		const bool reflected_edit = begin_reflected_edit(field, _object, old_value);

		auto clear_items = [&]<typename T>() {
			clear_reflected_container<T>(_object, field);
			if (reflected_edit)
				end_reflected_edit(field, _object, old_value);
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
		case reflected_value_type_e::i16:
			clear_items.template operator()<i16>();
			break;
		case reflected_value_type_e::i8:
			clear_items.template operator()<i8>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::enum32:
			clear_items.template operator()<u32>();
			break;
		case reflected_value_type_e::u64:
			clear_items.template operator()<u64>();
			break;
		case reflected_value_type_e::u16:
			clear_items.template operator()<u16>();
			break;
		case reflected_value_type_e::size_t:
			clear_items.template operator()<size_t>();
			break;
		case reflected_value_type_e::entity_guid:
			clear_items.template operator()<entity_guid_t>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			clear_items.template operator()<u8>();
			break;
			EDITOR_REFLECTED_RESOURCE_HANDLE_CASES
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
		if (field.flags.is_set(reflected_field_flags_read_only))
			return;

		ostream_t  old_value;
		const bool reflected_edit = begin_reflected_edit(field, _object, old_value);

		auto add_item = [&]<typename T>() {
			add_reflected_container_item<T>(_object, field);
			if (reflected_edit)
				end_reflected_edit(field, _object, old_value);
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
		case reflected_value_type_e::i16:
			add_item.template operator()<i16>();
			break;
		case reflected_value_type_e::i8:
			add_item.template operator()<i8>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::enum32:
			add_item.template operator()<u32>();
			break;
		case reflected_value_type_e::u64:
			add_item.template operator()<u64>();
			break;
		case reflected_value_type_e::u16:
			add_item.template operator()<u16>();
			break;
		case reflected_value_type_e::size_t:
			add_item.template operator()<size_t>();
			break;
		case reflected_value_type_e::entity_guid:
			add_item.template operator()<entity_guid_t>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			add_item.template operator()<u8>();
			break;
			EDITOR_REFLECTED_RESOURCE_HANDLE_CASES
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
		if (field.flags.is_set(reflected_field_flags_read_only))
			return;

		ostream_t  old_value;
		const bool reflected_edit = begin_reflected_edit(field, _object, old_value);

		auto remove_item = [&]<typename T>() {
			remove_reflected_container_item<T>(_object, field, index);
			if (reflected_edit)
				end_reflected_edit(field, _object, old_value);
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
		case reflected_value_type_e::i16:
			remove_item.template operator()<i16>();
			break;
		case reflected_value_type_e::i8:
			remove_item.template operator()<i8>();
			break;
		case reflected_value_type_e::u32:
		case reflected_value_type_e::enum32:
			remove_item.template operator()<u32>();
			break;
		case reflected_value_type_e::u64:
			remove_item.template operator()<u64>();
			break;
		case reflected_value_type_e::u16:
			remove_item.template operator()<u16>();
			break;
		case reflected_value_type_e::size_t:
			remove_item.template operator()<size_t>();
			break;
		case reflected_value_type_e::entity_guid:
			remove_item.template operator()<entity_guid_t>();
			break;
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			remove_item.template operator()<u8>();
			break;
			EDITOR_REFLECTED_RESOURCE_HANDLE_CASES
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

	bool editor_widget_reflect_type_t::begin_reflected_edit(const reflected_field_desc_t& field, void* object, ostream_t& old_value) const
	{
		if (_target.kind == editor_reflected_edit_target_kind_e::none)
			return false;
		return reflection_registry_t::get().serialize_field_to_stream(object, field, old_value);
	}

	void editor_widget_reflect_type_t::end_reflected_edit(const reflected_field_desc_t& field, void* object, ostream_t& old_value) const
	{
		if (_target.kind == editor_reflected_edit_target_kind_e::none)
			return;

		ostream_t new_value;
		if (!reflection_registry_t::get().serialize_field_to_stream(object, field, new_value))
			return;

		editor_reflected_field_edit_desc_t desc = {};
		desc.target								= _target;
		desc.type_id							= _type_id;
		desc.field_id							= field.id;
		editor_commands_reflection_t::edit_field(desc, old_value, new_value);
	}

	void editor_widget_reflect_type_t::begin_live_reflected_edit(reflected_control_t& control) const
	{
		if (control.edit_active)
			return;
		control.edit_old_value = new ostream_t();
		control.edit_active	   = begin_reflected_edit(*control.command_field, control.command_object, *control.edit_old_value);
		if (!control.edit_active)
		{
			delete control.edit_old_value;
			control.edit_old_value = nullptr;
		}
	}

	void editor_widget_reflect_type_t::submit_live_reflected_edit(reflected_control_t& control) const
	{
		if (!control.edit_active)
			return;
		end_reflected_edit(*control.command_field, control.command_object, *control.edit_old_value);
		delete control.edit_old_value;
		control.edit_old_value = nullptr;
		control.edit_active	   = false;
	}

	bool editor_widget_reflect_type_t::is_field_mixed(const reflected_field_desc_t& field, void* object) const
	{
		if (_target.kind != editor_reflected_edit_target_kind_e::world_components || _target.entity_count <= 1)
			return false;

		ostream_t first_value;
		if (!reflection_registry_t::get().serialize_field_to_stream(_type_id, field.id, object, first_value))
			return false;

		world_t&				 world = editor_app_t::get().get_runtime().get_world(_target.world);
		world_component_table_t* table = world.find_component_table(_target.type_id);
		SFG_ASSERT(table != nullptr);
		for (u32 i = 1; i < _target.entity_count; ++i)
		{
			SFG_ASSERT(ecs_t::table_has(table->table, _target.entities[i]));
			void*	  object_to_compare = ecs_t::table_get(table->table, _target.entities[i]);
			ostream_t value;
			if (!reflection_registry_t::get().serialize_field_to_stream(_type_id, field.id, object_to_compare, value))
				return false;
			if (value.get_size() != first_value.get_size())
				return true;
			if (value.get_size() != 0 && std::memcmp(value.get_raw(), first_value.get_raw(), value.get_size()) != 0)
				return true;
		}
		return false;
	}

	bool editor_widget_reflect_type_t::matches_reflected_command(const editor_command_reflected_field_edit_payload_t& payload) const
	{
		if (payload.type_id != _type_id)
			return false;

		if (payload.target.kind != _target.kind)
			return false;

		switch (_target.kind)
		{
		case editor_reflected_edit_target_kind_e::raw_object:
			return payload.target.object == _target.object;
		case editor_reflected_edit_target_kind_e::world_component:
			return payload.target.world == _target.world && payload.target.entity == _target.entity && payload.target.type_id == _target.type_id;
		case editor_reflected_edit_target_kind_e::world_components:
			return payload.target.world == _target.world && payload.target.type_id == _target.type_id;
		default:
			return false;
		}
	}

	bool editor_widget_reflect_type_t::refresh_reflected_field(sid_t field_id)
	{
		for (vector_control_t& control : _vector_controls)
		{
			if (control.field->id == field_id)
				return false;
		}

		for (reflected_control_t& control : _controls)
		{
			if (control.command_field->id == field_id)
				return refresh_reflected_control(control);
		}

		return false;
	}

	bool editor_widget_reflect_type_t::refresh_reflected_control(reflected_control_t& control)
	{
		if (control.command_field->type == reflected_value_type_e::vector || control.command_field->type == reflected_value_type_e::inplace_vector)
			return false;

		control.mixed = is_field_mixed(*control.command_field, control.command_object);

		switch (control.widget_type)
		{
		case reflected_control_widget_e::input: {
			editor_input_field_t& input = *static_cast<editor_input_field_t*>(control.widget);
			input.set_placeholder(control.mixed ? "Mixed" : nullptr);
			if (control.mixed)
			{
				input.set_text("");
				return true;
			}
			if (control.field->type == reflected_value_type_e::string)
				input.set_text(read_reflected_text(control.object, *control.field));
			else
				input.set_number(read_reflected_number(control.object, *control.field));
			return true;
		}
		case reflected_control_widget_e::checkbox: {
			editor_checkbox_t& checkbox = *static_cast<editor_checkbox_t*>(control.widget);
			if (control.mixed)
				checkbox.set_mixed(true);
			else
				checkbox.set_checked(read_reflected_bool(control.object, *control.field));
			return true;
		}
		case reflected_control_widget_e::color: {
			if (control.mixed)
				return false;
			editor_color_field_t& color = *static_cast<editor_color_field_t*>(control.widget);
			color.set_color(read_reflected_value<color_t>(control.object, *control.field).to_vector());
			return true;
		}
		case reflected_control_widget_e::vec2: {
			editor_vec2_field_t& vec = *static_cast<editor_vec2_field_t*>(control.widget);
			vec.set_mixed(control.mixed);
			if (control.mixed)
				return true;
			const sid_t type_id = control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
			if (type_id == type_id_t<vec2u_t>::value)
			{
				const vec2u_t value = read_reflected_value<vec2u_t>(control.object, *control.field);
				vec.set_value({static_cast<f32>(value.x), static_cast<f32>(value.y)});
			}
			else if (type_id == type_id_t<vec2u16_t>::value)
			{
				const vec2u16_t value = read_reflected_value<vec2u16_t>(control.object, *control.field);
				vec.set_value({static_cast<f32>(value.x), static_cast<f32>(value.y)});
			}
			else
				vec.set_value(read_reflected_value<vec2f_t>(control.object, *control.field));
			return true;
		}
		case reflected_control_widget_e::vec3: {
			editor_vec3_field_t& vec = *static_cast<editor_vec3_field_t*>(control.widget);
			vec.set_mixed(control.mixed);
			if (control.mixed)
				return true;
			const sid_t type_id = control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
			if (type_id == type_id_t<vec3u_t>::value)
			{
				const vec3u_t value = read_reflected_value<vec3u_t>(control.object, *control.field);
				vec.set_value({static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z)});
			}
			else
				vec.set_value(read_reflected_value<vec3f_t>(control.object, *control.field));
			return true;
		}
		case reflected_control_widget_e::vec4: {
			editor_vec4_field_t& vec = *static_cast<editor_vec4_field_t*>(control.widget);
			vec.set_mixed(control.mixed);
			if (control.mixed)
				return true;
			const sid_t type_id = control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
			if (type_id == type_id_t<vec4u_t>::value)
			{
				const vec4u_t value = read_reflected_value<vec4u_t>(control.object, *control.field);
				vec.set_value({static_cast<f32>(value.x), static_cast<f32>(value.y), static_cast<f32>(value.z), static_cast<f32>(value.w)});
			}
			else if (control.field->type == reflected_value_type_e::quat)
			{
				const quat_t value = read_reflected_value<quat_t>(control.object, *control.field);
				vec.set_value({value.x, value.y, value.z, value.w});
			}
			else
				vec.set_value(read_reflected_value<vec4f_t>(control.object, *control.field));
			return true;
		}
		case reflected_control_widget_e::asset_reference: {
			editor_widget_asset_reference_t& reference = *static_cast<editor_widget_asset_reference_t*>(control.widget);
			reference.set_mixed(control.mixed);
			return true;
		}
		case reflected_control_widget_e::entity_guid_reference: {
			editor_widget_entity_guid_reference_t& reference = *static_cast<editor_widget_entity_guid_reference_t*>(control.widget);
			reference.set_mixed(control.mixed);
			return true;
		}
		case reflected_control_widget_e::text_id: {
			editor_widget_text_id_t& text_id = *static_cast<editor_widget_text_id_t*>(control.widget);
			text_id.set_mixed(control.mixed);
			return true;
		}
		case reflected_control_widget_e::dropdown: {
			editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(control.widget);
			dropdown.set_mixed(control.mixed);
			return true;
		}
		default:
			return false;
		}
	}

	void editor_widget_reflect_type_t::on_number_changed(f32 value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		control.owner->begin_live_reflected_edit(control);
		write_reflected_number(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_text_changed(const char* value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		control.owner->begin_live_reflected_edit(control);
		write_reflected_text(control.object, *control.field, value);
	}

	void editor_widget_reflect_type_t::on_input_submitted(const char* text_value, f32 number_value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		if (control.mixed && !control.edit_active)
			return;
		if (control.field->type == reflected_value_type_e::string)
			write_reflected_text(control.object, *control.field, text_value);
		else
			write_reflected_number(control.object, *control.field, number_value);
		control.owner->submit_live_reflected_edit(control);
	}

	void editor_widget_reflect_type_t::on_checkbox_changed(bool checked, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		write_reflected_bool(control.object, *control.field, checked);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	void editor_widget_reflect_type_t::on_color_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		const sid_t			 type_id		= control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
		if (type_id == type_id_t<color_t>::value)
			write_reflected_value(control.object, *control.field, color_t{value.x, value.y, value.z, value.w});
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	void editor_widget_reflect_type_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		const sid_t			 type_id		= control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
		if (type_id == type_id_t<vec2u_t>::value)
			write_reflected_value(control.object, *control.field, vec2u_t{static_cast<u32>(value.x < 0.0f ? 0.0f : value.x), static_cast<u32>(value.y < 0.0f ? 0.0f : value.y)});
		else if (type_id == type_id_t<vec2u16_t>::value)
			write_reflected_value(control.object, *control.field, vec2u16_t{static_cast<u16>(value.x < 0.0f ? 0.0f : (value.x > 65535.0f ? 65535.0f : value.x)), static_cast<u16>(value.y < 0.0f ? 0.0f : (value.y > 65535.0f ? 65535.0f : value.y))});
		else
			write_reflected_value(control.object, *control.field, value);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	void editor_widget_reflect_type_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		const sid_t			 type_id		= control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
		if (type_id == type_id_t<vec3u_t>::value)
			write_reflected_value(control.object, *control.field, vec3u_t{static_cast<u32>(value.x < 0.0f ? 0.0f : value.x), static_cast<u32>(value.y < 0.0f ? 0.0f : value.y), static_cast<u32>(value.z < 0.0f ? 0.0f : value.z)});
		else
			write_reflected_value(control.object, *control.field, value);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	void editor_widget_reflect_type_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		const sid_t			 type_id		= control.field->value_type_id != 0 ? control.field->value_type_id : control.field->sub_type_id;
		if (type_id == type_id_t<vec4u_t>::value)
			write_reflected_value(control.object,
								  *control.field,
								  vec4u_t{static_cast<u32>(value.x < 0.0f ? 0.0f : value.x), static_cast<u32>(value.y < 0.0f ? 0.0f : value.y), static_cast<u32>(value.z < 0.0f ? 0.0f : value.z), static_cast<u32>(value.w < 0.0f ? 0.0f : value.w)});
		else if (control.field->type == reflected_value_type_e::quat)
			write_reflected_value(control.object, *control.field, quat_t{value.x, value.y, value.z, value.w});
		else
			write_reflected_value(control.object, *control.field, value);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	sid_t editor_widget_reflect_type_t::on_asset_selected(void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		return read_reflected_value<sid_t>(control.object, *control.field);
	}

	void editor_widget_reflect_type_t::on_asset_pressed(sid_t guid, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		write_reflected_value(control.object, *control.field, guid);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	entity_guid_t editor_widget_reflect_type_t::on_entity_selected(void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		return read_reflected_value<entity_guid_t>(control.object, *control.field);
	}

	void editor_widget_reflect_type_t::on_entity_pressed(entity_guid_t guid, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		ostream_t			 old_value;
		const bool			 reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		write_reflected_value(control.object, *control.field, guid);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
	}

	u32 editor_widget_reflect_type_t::on_text_id_selected(void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		return read_reflected_value<u32>(control.object, *control.field);
	}

	void editor_widget_reflect_type_t::on_text_id_submitted(const char* text, void* user_data)
	{
		reflected_control_t& control = *static_cast<reflected_control_t*>(user_data);
		if (control.owner->_target.kind == editor_reflected_edit_target_kind_e::none)
			return;
		if (control.command_field->flags.is_set(reflected_field_flags_read_only))
			return;

		editor_reflected_field_edit_desc_t desc = {};
		desc.target								= control.owner->_target;
		desc.type_id							= control.owner->_type_id;
		desc.field_id							= control.command_field->id;

		const world_handle_t world_handle = control.owner->_target.world.is_null() ? editor_app_t::get().get_main_world() : control.owner->_target.world;
		world_t&			 world		  = editor_app_t::get().get_runtime().get_world(world_handle);
		const u32			 old_text_id  = read_reflected_value<u32>(control.command_object, *control.command_field);
		const char*			 old_text	  = world.get_text(old_text_id);
		editor_commands_reflection_t::edit_text_id_field(desc, world_handle, old_text != nullptr ? old_text : "", text != nullptr ? text : "");
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
		ostream_t  old_value;
		const bool reflected_edit = control.owner->begin_reflected_edit(*control.command_field, control.command_object, old_value);
		write_reflected_enum(control.object, *control.field, enum_values.data[value].value);
		if (reflected_edit)
			control.owner->end_reflected_edit(*control.command_field, control.command_object, old_value);
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

	void editor_widget_reflect_type_t::on_command_system_changed(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_widget_reflect_type_t&						 widget	 = *static_cast<editor_widget_reflect_type_t*>(user_data);
		const editor_command_reflected_field_edit_payload_t* payload = editor_commands_reflection_t::get_payload(system, command);
		if (payload != nullptr && widget.matches_reflected_command(*payload))
		{
			if (!widget.refresh_reflected_field(payload->field_id))
				widget.rebuild_reflected_controls();
		}
	}

	void editor_widget_reflect_type_t::clear_reflected_controls()
	{
		for (size_t i = _text_ids.size(); i > 0; --i)
		{
			_text_ids[i - 1]->uninit();
			delete _text_ids[i - 1];
		}
		_text_ids.resize(0);

		for (size_t i = _entity_references.size(); i > 0; --i)
		{
			_entity_references[i - 1]->uninit();
			delete _entity_references[i - 1];
		}
		_entity_references.resize(0);

		for (size_t i = _asset_references.size(); i > 0; --i)
		{
			_asset_references[i - 1]->uninit();
			delete _asset_references[i - 1];
		}
		_asset_references.resize(0);

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

		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		for (size_t i = _rows.size(); i > 0; --i)
		{
			if (tooltip_controller != nullptr && _rows[i - 1].label != NULL_WIDGET)
				tooltip_controller->clear_tooltip(_rows[i - 1].label);
			_ui->deallocate_widget(_rows[i - 1].row);
		}
		_rows.resize(0);
		for (reflected_control_t& control : _controls)
		{
			delete control.edit_old_value;
			control.edit_old_value = nullptr;
			control.edit_active	   = false;
		}
		_controls.resize(0);
		_vector_controls.resize(0);
		_vector_item_controls.resize(0);
		_vector_item_fields.resize(0);
	}
}
