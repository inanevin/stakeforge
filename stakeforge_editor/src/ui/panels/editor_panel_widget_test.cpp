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
#include "ui/panels/editor_panel_widget_test.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2u.hpp>
#include <sfg/math/vec3u.hpp>
#include <sfg/math/vec4u.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

#include <cstddef>

namespace sfg
{
#define EDITOR_WIDGET_TEST_DUMMY_ENUM_TYPE_ID  "editor_widget_test_dummy_enum"_hs
#define EDITOR_WIDGET_TEST_DUMMY_ENUM8_TYPE_ID "editor_widget_test_dummy_enum8"_hs

	namespace
	{
		enum class dummy_enum_e : u32
		{
			first,
			second,
			third,
		};

		enum class dummy_enum8_e : u8
		{
			first,
			second,
			third,
		};

		struct dummy_struct_t
		{
			static inline constexpr sid_t TYPE_ID = "editor_widget_test_dummy_struct"_hs;

			vector_t<string_t>			  dummy_string_vector		  = {"Alpha", "Beta", "Gamma"};
			vector_t<f32>				  dummy_f32_vector			  = {1.0f, 2.0f, 3.0f};
			vector_t<vec3f_t>			  dummy_vec3_vector			  = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
			vector_t<u32>				  dummy_u32_vector			  = {7, 8, 9};
			vector_t<u8>				  dummy_bool_vector			  = {1, 0, 1};
			inplace_vector_t<string_t, 4> dummy_inplace_string_vector = {"North", "East"};
			inplace_vector_t<f32, 4>	  dummy_inplace_f32_vector	  = {4.0f, 5.0f};
			inplace_vector_t<vec4f_t, 4>  dummy_inplace_vec4_vector	  = {{1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}};
			inplace_vector_t<u32, 4>	  dummy_inplace_enum_vector	  = {0, 2};
			color_t						  dummy_color				  = {0.8f, 0.2f, 0.6f, 1.0f};
			quat_t						  dummy_quat				  = quat_t::identity;
			char						  dummy_string[64]			  = "Stakeforge";
			vec4f_t						  dummy_vec4				  = {1.0f, 2.0f, 3.0f, 4.0f};
			vec4u_t						  dummy_vec4u				  = {1, 2, 3, 4};
			vec3f_t						  dummy_vec3				  = {1.0f, 2.0f, 3.0f};
			vec3u_t						  dummy_vec3u				  = {1, 2, 3};
			vec2f_t						  dummy_vec2				  = {1.0f, 2.0f};
			vec2u_t						  dummy_vec2u				  = {1, 2};
			f32							  dummy_f32					  = 12.5f;
			u32							  dummy_u32					  = 42;
			i32							  dummy_i32					  = -7;
			dummy_enum_e				  dummy_enum				  = dummy_enum_e::second;
			dummy_enum8_e				  dummy_enum8				  = dummy_enum8_e::second;
			u8							  dummy_u8					  = 3;
		};

		void register_dummy_struct_reflection()
		{
			static const reflected_enum_value_desc_t enum_values[] = {
				{.name = "first", .display_name = "First", .value = static_cast<i64>(dummy_enum_e::first)},
				{.name = "second", .display_name = "Second", .value = static_cast<i64>(dummy_enum_e::second)},
				{.name = "third", .display_name = "Third", .value = static_cast<i64>(dummy_enum_e::third)},
			};

			static const reflected_enum_value_desc_t enum8_values[] = {
				{.name = "first", .display_name = "First", .value = static_cast<i64>(dummy_enum8_e::first)},
				{.name = "second", .display_name = "Second", .value = static_cast<i64>(dummy_enum8_e::second)},
				{.name = "third", .display_name = "Third", .value = static_cast<i64>(dummy_enum8_e::third)},
			};

			static const reflected_field_desc_t fields[] = {
				{.name = "dummy_f32", .display_name = "Float", .type = reflected_value_type_e::f32, .offset = offsetof(dummy_struct_t, dummy_f32), .size = sizeof(f32), .min = 0.0f, .max = 100.0f, .flags = reflected_field_flags_clamped},
				{.name = "dummy_u32", .display_name = "U32", .type = reflected_value_type_e::u32, .offset = offsetof(dummy_struct_t, dummy_u32), .size = sizeof(u32)},
				{.name = "dummy_i32", .display_name = "I32", .type = reflected_value_type_e::i32, .offset = offsetof(dummy_struct_t, dummy_i32), .size = sizeof(i32)},
				{.name = "dummy_u8", .display_name = "U8", .type = reflected_value_type_e::u8, .offset = offsetof(dummy_struct_t, dummy_u8), .size = sizeof(u8)},
				{.name = "dummy_vec2", .display_name = "Vec2", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2f_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec2), .size = sizeof(vec2f_t)},
				{.name = "dummy_vec3", .display_name = "Vec3", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec3), .size = sizeof(vec3f_t)},
				{.name = "dummy_vec4", .display_name = "Vec4", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec4f_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec4), .size = sizeof(vec4f_t)},
				{.name = "dummy_vec2u", .display_name = "Vec2U", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec2u), .size = sizeof(vec2u_t)},
				{.name = "dummy_vec3u", .display_name = "Vec3U", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3u_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec3u), .size = sizeof(vec3u_t)},
				{.name = "dummy_vec4u", .display_name = "Vec4U", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec4u_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec4u), .size = sizeof(vec4u_t)},
				{.name = "dummy_color", .display_name = "Color", .type = reflected_value_type_e::object, .value_type_id = type_id_t<color_t>::value, .offset = offsetof(dummy_struct_t, dummy_color), .size = sizeof(color_t)},
				{.name = "dummy_quat", .display_name = "Quat", .type = reflected_value_type_e::quat, .offset = offsetof(dummy_struct_t, dummy_quat), .size = sizeof(quat_t)},
				{.name = "dummy_string", .display_name = "String", .type = reflected_value_type_e::string, .offset = offsetof(dummy_struct_t, dummy_string), .size = sizeof(dummy_struct_t::dummy_string)},
				{.name = "dummy_enum", .display_name = "Enum", .type = reflected_value_type_e::enum32, .sub_type_id = EDITOR_WIDGET_TEST_DUMMY_ENUM_TYPE_ID, .offset = offsetof(dummy_struct_t, dummy_enum), .size = sizeof(dummy_enum_e)},
				{.name = "dummy_enum8", .display_name = "Enum8", .type = reflected_value_type_e::enum8, .sub_type_id = EDITOR_WIDGET_TEST_DUMMY_ENUM8_TYPE_ID, .offset = offsetof(dummy_struct_t, dummy_enum8), .size = sizeof(dummy_enum8_e)},
				{.name = "dummy_f32_vector", .display_name = "Float Vector", .type = reflected_value_type_e::vector, .sub_type_id = "f32"_hs, .offset = offsetof(dummy_struct_t, dummy_f32_vector), .size = sizeof(vector_t<f32>)},
				{.name = "dummy_string_vector", .display_name = "String Vector", .type = reflected_value_type_e::vector, .sub_type_id = "string"_hs, .offset = offsetof(dummy_struct_t, dummy_string_vector), .size = sizeof(vector_t<string_t>)},
				{.name = "dummy_vec3_vector", .display_name = "Vec3 Vector", .type = reflected_value_type_e::vector, .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(dummy_struct_t, dummy_vec3_vector), .size = sizeof(vector_t<vec3f_t>)},
				{.name = "dummy_u32_vector", .display_name = "U32 Vector", .type = reflected_value_type_e::vector, .sub_type_id = "u32"_hs, .offset = offsetof(dummy_struct_t, dummy_u32_vector), .size = sizeof(vector_t<u32>)},
				{.name = "dummy_bool_vector", .display_name = "Bool Vector", .type = reflected_value_type_e::vector, .sub_type_id = "bool8"_hs, .offset = offsetof(dummy_struct_t, dummy_bool_vector), .size = sizeof(vector_t<u8>)},
				{.name		   = "dummy_inplace_f32_vector",
				 .display_name = "Static Float Vector",
				 .type		   = reflected_value_type_e::inplace_vector,
				 .sub_type_id  = "f32"_hs,
				 .offset	   = offsetof(dummy_struct_t, dummy_inplace_f32_vector),
				 .size		   = sizeof(inplace_vector_t<f32, 4>),
				 .capacity	   = 4},
				{.name		   = "dummy_inplace_string_vector",
				 .display_name = "Static String Vector",
				 .type		   = reflected_value_type_e::inplace_vector,
				 .sub_type_id  = "string"_hs,
				 .offset	   = offsetof(dummy_struct_t, dummy_inplace_string_vector),
				 .size		   = sizeof(inplace_vector_t<string_t, 4>),
				 .capacity	   = 4},
				{.name		   = "dummy_inplace_vec4_vector",
				 .display_name = "Static Vec4 Vector",
				 .type		   = reflected_value_type_e::inplace_vector,
				 .sub_type_id  = type_id_t<vec4f_t>::value,
				 .offset	   = offsetof(dummy_struct_t, dummy_inplace_vec4_vector),
				 .size		   = sizeof(inplace_vector_t<vec4f_t, 4>),
				 .capacity	   = 4},
				{.name		   = "dummy_inplace_enum_vector",
				 .display_name = "Static Enum Vector",
				 .type		   = reflected_value_type_e::inplace_vector,
				 .sub_type_id  = EDITOR_WIDGET_TEST_DUMMY_ENUM_TYPE_ID,
				 .offset	   = offsetof(dummy_struct_t, dummy_inplace_enum_vector),
				 .size		   = sizeof(inplace_vector_t<u32, 4>),
				 .capacity	   = 4},
			};

			reflection_registry_t& registry = reflection_registry_t::get();
			if (registry.find_type(dummy_struct_t::TYPE_ID) != nullptr)
				return;

			registry.register_type({
				.enum_values = {.data = enum_values, .size = std::size(enum_values)},
				.name		 = "editor_widget_test_dummy_enum",
				.type_id	 = EDITOR_WIDGET_TEST_DUMMY_ENUM_TYPE_ID,
				.size		 = sizeof(dummy_enum_e),
				.alignment	 = alignof(dummy_enum_e),
			});

			registry.register_type({
				.enum_values = {.data = enum8_values, .size = std::size(enum8_values)},
				.name		 = "editor_widget_test_dummy_enum8",
				.type_id	 = EDITOR_WIDGET_TEST_DUMMY_ENUM8_TYPE_ID,
				.size		 = sizeof(dummy_enum8_e),
				.alignment	 = alignof(dummy_enum8_e),
			});

			registry.register_type({
				.fields	   = {.data = fields, .size = std::size(fields)},
				.name	   = "editor_widget_test_dummy_struct",
				.category  = "editor",
				.type_id   = dummy_struct_t::TYPE_ID,
				.size	   = sizeof(dummy_struct_t),
				.alignment = alignof(dummy_struct_t),
			});
		}

		dummy_struct_t g_dummy_struct = {};
	}

	editor_panel_widget_test_t::editor_panel_widget_test_t()
	{
		set_type(editor_panel_type_e::widget_test);
		set_title(editor_panel_type_to_string(editor_panel_type_e::widget_test));
	}

	void editor_panel_widget_test_t::init(ui::ui_context& ui, ui::widget_id_t parent)
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
		column_in.flags			   = ui::wf_visible;
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		_spinner_holder = ui.allocate_widget();
		ui.set_widget_debug_name(_spinner_holder, "inspector_spinner_holder");
		tree.attach(_column, _spinner_holder);

		ui::layout_in_t& spinner_holder_in = tree.in(_spinner_holder);
		spinner_holder_in.flags			   = ui::wf_visible;
		spinner_holder_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		spinner_holder_in.pos_mode_y	   = ui::pos_mode_e::flow;
		spinner_holder_in.pos_value.x	   = 0.5f;
		spinner_holder_in.anchor_x		   = ui::anchor_e::center;
		spinner_holder_in.size_mode_x	   = ui::axis_mode_e::fixed;
		spinner_holder_in.size_mode_y	   = ui::axis_mode_e::fixed;
		spinner_holder_in.size_value	   = {theme.item_height * 4.0f, theme.item_height * 4.0f};

		_spinner.init(ui, _spinner_holder, {.outer_color = theme.color_accent0, .inner_color = theme.color_accent1});

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

		_reflect_fold.init(ui, _column, {.label = "test fold reflection"});
		_reflect_type.init(ui, _reflect_fold.get_body());
		tree.in(_reflect_type.get_root()).size_mode_y = ui::axis_mode_e::sum_children;
		register_dummy_struct_reflection();
		_reflect_type.set_reflected_obj(&g_dummy_struct, dummy_struct_t::TYPE_ID);
	}

	void editor_panel_widget_test_t::uninit()
	{
		_reflect_type.uninit();
		_reflect_fold.uninit();
		_spinner.uninit();
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

		_column			= NULL_WIDGET;
		_spinner_holder = NULL_WIDGET;
	}

	void editor_panel_widget_test_t::on_text_changed(const char*, void*)
	{
	}

	void editor_panel_widget_test_t::on_number_changed(f32 value, void* user_data)
	{
		*static_cast<f32*>(user_data) = value;
	}

	void editor_panel_widget_test_t::on_checkbox_changed(bool checked, void* user_data)
	{
		*static_cast<bool*>(user_data) = checked;
	}

	void editor_panel_widget_test_t::on_color_changed(const vec4f_t& value, void* user_data)
	{
		*static_cast<vec4f_t*>(user_data) = value;
	}

	void editor_panel_widget_test_t::on_vec2_changed(const vec2f_t& value, void* user_data)
	{
		*static_cast<vec2f_t*>(user_data) = value;
	}

	void editor_panel_widget_test_t::on_vec3_changed(const vec3f_t& value, void* user_data)
	{
		*static_cast<vec3f_t*>(user_data) = value;
	}

	void editor_panel_widget_test_t::on_vec4_changed(const vec4f_t& value, void* user_data)
	{
		*static_cast<vec4f_t*>(user_data) = value;
	}
}
