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

#include "ui/widgets/editor_widget_reflection.hpp"
#include "assets/editor_asset_type.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "editor_widget_checkbox.hpp"
#include "editor_widget_fold_label.hpp"
#include "editor_widget_input_field.hpp"
#include "editor_widget_reference.hpp"
#include "editor_widgets_misc.hpp"
#include "editor_widgets_vec_fields.hpp"
#include "editor_widgets_dividers.hpp"
#include "editor_widgets_icons.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <cstdio>

namespace sfg
{
	namespace
	{
		bool is_container_type_allowed(reflected_value_type_e type, sid_t sub_type_id)
		{
			switch (type)
			{
			case reflected_value_type_e::boolean:
			case reflected_value_type_e::string:
			case reflected_value_type_e::char_array:
			case reflected_value_type_e::f32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::i32:
			case reflected_value_type_e::u16:
			case reflected_value_type_e::i16:
			case reflected_value_type_e::u8:
			case reflected_value_type_e::i8:
			case reflected_value_type_e::u64:
			case reflected_value_type_e::i64:
				return true;
			case reflected_value_type_e::object:
				return sub_type_id == type_id_t<vec2f_t>::value || sub_type_id == type_id_t<vec3f_t>::value || sub_type_id == type_id_t<vec4f_t>::value || sub_type_id == type_id_t<quat_t>::value;
			default:
				return false;
			}
		}
	}

	void editor_widget_reflection_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_reflection_config_t& config)
	{
		_ui = &ui;

		ui::layout_tree_t& tree	 = ui.get_tree();
		editor_theme_t&	   theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reflection");
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
		//	root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		set_reflection(config);
	}

	void editor_widget_reflection_t::uninit()
	{
		clear_widgets();
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_widget_reflection_t::set_reflection(const editor_widget_reflection_config_t& config)
	{
		clear_widgets();

		const reflected_type_t* type = reflection_registry_t::get().find_type(config.type_id);
		if (type == nullptr)
			return;

		if (config.objects.size == 0)
			return;
		editor_theme_t& theme = editor_theme_t::get();

		create_fields(_root, config.objects, config.type_id, config.world, true, false, theme.margin_horizontal, true);
	}

	void editor_widget_reflection_t::create_fields(ui::widget_id_t parent, span_t<void*> objects, sid_t type_id, world_handle_t world, bool track_rows, bool sub_item, f32 indentation, bool add_divider)
	{
		const reflected_type_t* type = reflection_registry_t::get().find_type(type_id);
		if (type == nullptr)
			return;

		const u32 field_start = type->fields.start;
		const u32 field_end	  = type->fields.end;
		for (u32 i = field_start; i < field_end; i++)
		{
			const reflected_field_t* field = reflection_registry_t::get().get_field(i);
			if (field == nullptr || field->flags.is_set(reflected_field_flags_e::reflected_field_flag_no_ui))
				continue;

			switch (field->value_type)
			{
			case reflected_value_type_e::boolean: {
				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_checkbox(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, indentation);
				break;
			}
			case reflected_value_type_e::string:
			case reflected_value_type_e::char_array: {
				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_input_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, indentation);
				break;
			}
			case reflected_value_type_e::object: {
				frame_vector_t<void*> object_fields;
				object_fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					object_fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_object(parent, field, {.data = object_fields.data(), .size = object_fields.size()}, world, track_rows, sub_item, indentation);
				break;
			}
			case reflected_value_type_e::container: {
				frame_vector_t<void*> container_fields;
				container_fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					container_fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_container(parent, field, {.data = container_fields.data(), .size = container_fields.size()}, world, sub_item, indentation);
				break;
			}
			case reflected_value_type_e::f32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::i32:
			case reflected_value_type_e::u16:
			case reflected_value_type_e::i16:
			case reflected_value_type_e::u8:
			case reflected_value_type_e::i8:
			case reflected_value_type_e::u64:
			case reflected_value_type_e::i64: {

				if (field->value_type == reflected_value_type_e::u64)
				{
					frame_vector_t<u64*> reference_fields;
					reference_fields.reserve(objects.size);
					for (size_t idx = 0; idx < objects.size; ++idx)
						reference_fields.push_back(reinterpret_cast<u64*>(static_cast<u8*>(objects.data[idx]) + field->offset));

					if (create_reference(parent, field, {.data = reference_fields.data(), .size = reference_fields.size()}, world, track_rows, sub_item, indentation))
						break;
				}

				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_input_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, indentation);
				break;
			}
			}

			if (add_divider && i != field_end - 1)
				_dividers.push_back(editor_dividers_t::add_divider_hor(*_ui, parent, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none));
		}
	}

	void editor_widget_reflection_t::fit_control(ui::widget_id_t widget)
	{
		ui::layout_in_t& input_in = _ui->get_tree().in(widget);
		input_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		input_in.size_mode_y	  = ui::axis_mode_e::fixed;
		input_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		input_in.anchor_y		  = ui::anchor_e::center;
		input_in.pos_value.y	  = 0.5f;
		input_in.size_value		  = {1.0f, editor_theme_t::get().item_height};
	}

	void editor_widget_reflection_t::create_checkbox(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_checkbox_t* checkbox = new editor_checkbox_t();
		checkbox->init(*_ui,
					   row.right,
					   {
						   .field = {.fields = fields},
					   });
		ui::layout_in_t& checkbox_in = _ui->get_tree().in(checkbox->get_root());
		checkbox_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		checkbox_in.anchor_y		 = ui::anchor_e::center;
		checkbox_in.pos_value.y		 = 0.5f;
		if (sub_item)
			install_sub_item_button(row.right);
		_checkboxes.push_back(checkbox);
		if (track_row)
			_rows.push_back(row.row);
	}

	void editor_widget_reflection_t::create_input_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_input_field_field_type_e input_type = editor_input_field_field_type_e::pod_number;
		if (field->value_type == reflected_value_type_e::string)
			input_type = editor_input_field_field_type_e::string;
		else if (field->value_type == reflected_value_type_e::char_array)
			input_type = editor_input_field_field_type_e::char_array;

		const bool is_slider	 = field->flags.is_set(reflected_field_flags_e::reflected_field_flag_clamped);
		const bool integer		 = field->value_type != reflected_value_type_e::f32;
		const bool signed_number = field->value_type == reflected_value_type_e::i64 || field->value_type == reflected_value_type_e::i32 || field->value_type == reflected_value_type_e::i16 || field->value_type == reflected_value_type_e::i8;

		editor_input_field_t* input = new editor_input_field_t();
		input->init(*_ui,
					row.right,
					{
						.field =
							{
								.type		= input_type,
								.fields		= fields,
								.field_size = field->size,
								.is_slider	= is_slider,
							},
						.placeholder = field->name,
						.increment	 = integer ? 1.0f : 0.1f,
						.min_value	 = signed_number ? -1.0f : 0.0f,
						.max_value	 = 1.0f,
					});
		fit_control(input->get_root());
		if (sub_item)
			install_sub_item_button(row.right, input->get_root());
		_inputs.push_back(input);
		if (track_row)
			_rows.push_back(row.row);
	}

	bool editor_widget_reflection_t::create_reference(ui::widget_id_t parent, const reflected_field_t* field, span_t<u64*> fields, world_handle_t world, bool track_row, bool sub_item, f32 indentation)
	{
		const resource_type_e	  resource_type	   = resource_type_from_reflection_sub_type_id(field->sub_type_id);
		const editor_asset_type_e asset_type	   = editor_asset_type_from_resource_type(resource_type);
		const bool				  entity_reference = field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID;
		if (!entity_reference && asset_type == editor_asset_type_e::invalid)
			return false;

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_widget_reference_t* reference = new editor_widget_reference_t();
		reference->init(*_ui,
						row.right,
						{
							.fields		= fields,
							.world		= world,
							.asset_type = asset_type,
							.type		= entity_reference ? editor_widget_reference_type_e::entity : editor_widget_reference_type_e::asset,
						});
		fit_control(reference->get_root());
		if (sub_item)
			install_sub_item_button(row.right, reference->get_root());
		_references.push_back(reference);
		if (track_row)
			_rows.push_back(row.row);
		return true;
	}

	bool editor_widget_reflection_t::create_vector_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation)
	{
		if (field->sub_type_id == type_id_t<vec2f_t>::value)
		{
			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
			install_tooltip(row.label, field->tooltip);

			frame_vector_t<vec2f_t*> vector_fields;
			vector_fields.reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				vector_fields.push_back(reinterpret_cast<vec2f_t*>(fields.data[i]));

			editor_vec2_field_t* vec = new editor_vec2_field_t();
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}});
			fit_control(vec->get_root());
			if (sub_item)
				install_sub_item_button(row.right, vec->get_root());
			_vec2_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}
		if (field->sub_type_id == type_id_t<vec3f_t>::value)
		{
			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
			install_tooltip(row.label, field->tooltip);

			frame_vector_t<vec3f_t*> vector_fields;
			vector_fields.reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				vector_fields.push_back(reinterpret_cast<vec3f_t*>(fields.data[i]));

			editor_vec3_field_t* vec = new editor_vec3_field_t();
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}});
			fit_control(vec->get_root());
			if (sub_item)
				install_sub_item_button(row.right, vec->get_root());
			_vec3_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}
		if (field->sub_type_id == type_id_t<vec4f_t>::value || field->sub_type_id == type_id_t<quat_t>::value)
		{
			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
			install_tooltip(row.label, field->tooltip);

			frame_vector_t<vec4f_t*> vector_fields;
			vector_fields.reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				vector_fields.push_back(reinterpret_cast<vec4f_t*>(fields.data[i]));

			editor_vec4_field_t* vec = new editor_vec4_field_t();
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}});
			fit_control(vec->get_root());
			if (sub_item)
				install_sub_item_button(row.right, vec->get_root());
			_vec4_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}

		return false;
	}

	void editor_widget_reflection_t::create_object(ui::widget_id_t parent, const reflected_field_t* field, span_t<void*> objects, world_handle_t world, bool track_row, bool sub_item, f32 indentation)
	{
		frame_vector_t<u8*> fields;
		fields.reserve(objects.size);
		for (size_t i = 0; i < objects.size; ++i)
			fields.push_back(static_cast<u8*>(objects.data[i]));

		if (create_vector_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_row, sub_item, indentation))
			return;

		editor_widget_fold_label_t* fold = new editor_widget_fold_label_t();
		fold->init(*_ui,
				   parent,
				   {
					   .label		 = field->display_name ? field->display_name : "missing_display_name",
					   .button_style = sub_item ? editor_widget_fold_label_button_style_e::container_item_buttons : editor_widget_fold_label_button_style_e::none,
					   .indentation	 = indentation,
					   .sub_item	 = sub_item,
				   });
		install_tooltip(fold->get_root(), field->tooltip);
		if (sub_item)
			install_tooltip(fold->get_remove_button(), "Remove Element");
		_fold_labels.push_back(fold);
		create_fields(fold->get_body(), objects, field->sub_type_id, world, false, true, indentation + editor_theme_t::get().margin_horizontal, false);
	}

	void editor_widget_reflection_t::create_container(ui::widget_id_t parent, const reflected_field_t* const field, span_t<void*> containers, world_handle_t world, bool sub_item, f32 indentation)
	{
		const reflected_value_type_e element_value_type	 = field->container_ops.element_value_type;
		const sid_t					 element_sub_type_id = field->container_ops.element_sub_type_id;
		SFG_ASSERT(is_container_type_allowed(element_value_type, element_sub_type_id));

		editor_widget_fold_label_t* fold = new editor_widget_fold_label_t();
		fold->init(*_ui,
				   parent,
				   {
					   .label		 = field->display_name ? field->display_name : "missing_display_name",
					   .button_style = editor_widget_fold_label_button_style_e::container_buttons,
					   .indentation	 = indentation,
					   .sub_item	 = sub_item,
				   });
		install_tooltip(fold->get_root(), field->tooltip);
		install_tooltip(fold->get_add_button(), "Add Element");
		install_tooltip(fold->get_reset_button(), "Reset Container");

		ui::listener_bundle_t add_listener = {};
		add_listener.on_click			   = on_container_add;
		_ui->get_input().set_listener(fold->get_add_button(), add_listener);

		ui::listener_bundle_t reset_listener = {};
		reset_listener.on_click				 = on_container_reset;
		_ui->get_input().set_listener(fold->get_reset_button(), reset_listener);
		_fold_labels.push_back(fold);

		create_container_elements(fold->get_body(), field, containers, world, indentation);
	}

	void editor_widget_reflection_t::create_container_elements(ui::widget_id_t parent, const reflected_field_t* const field, span_t<void*> containers, world_handle_t world, f32 indentation)
	{
		const reflected_value_type_e element_value_type	 = field->container_ops.element_value_type;
		const sid_t					 element_sub_type_id = field->container_ops.element_sub_type_id;

		size_t element_count = field->container_ops.get_element_size_fn(containers.data[0]);
		for (size_t i = 1; i < containers.size; ++i)
		{
			const size_t container_size = field->container_ops.get_element_size_fn(containers.data[i]);
			if (container_size < element_count)
				element_count = container_size;
		}

		for (u32 element_index = 0; element_index < element_count; ++element_index)
		{
			char element_label[32] = {};
			std::snprintf(element_label, sizeof(element_label), "Element %u", element_index);

			reflected_field_t element_field{
				.name		  = "Element",
				.display_name = element_label,
				.sub_type_id  = element_sub_type_id,
				.size		  = field->container_ops.element_value_size,
				.value_type	  = element_value_type,
			};

			switch (element_value_type)
			{
			case reflected_value_type_e::boolean: {
				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_checkbox(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, indentation + editor_theme_t::get().margin_horizontal);
				break;
			}
			case reflected_value_type_e::string:
			case reflected_value_type_e::char_array: {
				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_input_field(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, indentation + editor_theme_t::get().margin_horizontal);
				break;
			}
			case reflected_value_type_e::object: {
				frame_vector_t<void*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_object(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, world, false, true, indentation + editor_theme_t::get().margin_horizontal);
				break;
			}
			case reflected_value_type_e::f32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::i32:
			case reflected_value_type_e::u16:
			case reflected_value_type_e::i16:
			case reflected_value_type_e::u8:
			case reflected_value_type_e::i8:
			case reflected_value_type_e::u64:
			case reflected_value_type_e::i64: {
				if (element_value_type == reflected_value_type_e::u64)
				{
					frame_vector_t<u64*> reference_fields;
					reference_fields.reserve(containers.size);
					for (size_t i = 0; i < containers.size; ++i)
						reference_fields.push_back(reinterpret_cast<u64*>(field->container_ops.get_element_ptr_fn(containers.data[i], element_index)));

					if (create_reference(parent, &element_field, {.data = reference_fields.data(), .size = reference_fields.size()}, world, false, true, indentation + editor_theme_t::get().margin_horizontal))
						break;
				}

				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_input_field(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, indentation + editor_theme_t::get().margin_horizontal);
				break;
			}
			default:
				SFG_ASSERT(false);
				break;
			}
		}
	}

	void editor_widget_reflection_t::install_sub_item_button(ui::widget_id_t parent, ui::widget_id_t control)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		if (control != NULL_WIDGET)
		{
			ui::layout_in_t& control_in = tree.in(control);
			control_in.size_mode_x		= ui::axis_mode_e::fill;
			control_in.size_value.x		= 1.0f;
		}
		else
		{
			const ui::widget_id_t filler = _ui->allocate_widget();
			_ui->set_widget_debug_name(filler, "sub_item_button_filler");
			tree.attach(parent, filler);

			ui::layout_in_t& filler_in = tree.in(filler);
			filler_in.flags			   = ui::wf_visible;
			filler_in.size_mode_x	   = ui::axis_mode_e::fill;
			filler_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			filler_in.size_value	   = {1.0f, 1.0f};
		}

		const ui::widget_id_t remove_button					  = editor_icon_widgets_t::add_naked_icon_button(*_ui, parent, ICON_CROSS, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
		tree.draw_order(remove_button)						  = tree.draw_order_const(parent) + 1;
		tree.draw_order(tree.node(remove_button).first_child) = tree.draw_order_const(remove_button);
		install_tooltip(remove_button, "Remove Element");
	}

	void editor_widget_reflection_t::on_container_add(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e, void*)
	{
	}

	void editor_widget_reflection_t::on_container_reset(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e, void*)
	{
	}

	void editor_widget_reflection_t::install_tooltip(ui::widget_id_t owner, const char* text)
	{
		if (text == nullptr || text[0] == '\0')
			return;

		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		if (tooltip_controller == nullptr)
			return;

		editor_tooltip_desc_t tooltip = {};
		tooltip.text				  = text;
		tooltip_controller->set_tooltip(owner, tooltip);
		_tooltip_owners.push_back(owner);
	}

	void editor_widget_reflection_t::clear_tooltips()
	{
		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		if (tooltip_controller != nullptr)
		{
			for (ui::widget_id_t owner : _tooltip_owners)
				tooltip_controller->clear_tooltip(owner);
		}
		_tooltip_owners.resize(0);
	}

	void editor_widget_reflection_t::clear_widgets()
	{
		clear_tooltips();

		for (editor_input_field_t* input : _inputs)
		{
			input->uninit();
			delete input;
		}
		for (editor_checkbox_t* checkbox : _checkboxes)
		{
			checkbox->uninit();
			delete checkbox;
		}
		for (editor_vec2_field_t* vec : _vec2_fields)
		{
			vec->uninit();
			delete vec;
		}
		for (editor_vec3_field_t* vec : _vec3_fields)
		{
			vec->uninit();
			delete vec;
		}
		for (editor_vec4_field_t* vec : _vec4_fields)
		{
			vec->uninit();
			delete vec;
		}
		for (editor_widget_reference_t* reference : _references)
		{
			reference->uninit();
			delete reference;
		}
		for (size_t i = _fold_labels.size(); i > 0; --i)
		{
			_fold_labels[i - 1]->uninit();
			delete _fold_labels[i - 1];
		}
		for (ui::widget_id_t row : _rows)
			_ui->deallocate_widget(row);
		for (ui::widget_id_t divider : _dividers)
			_ui->deallocate_widget(divider);
		_inputs.resize(0);
		_checkboxes.resize(0);
		_vec2_fields.resize(0);
		_vec3_fields.resize(0);
		_vec4_fields.resize(0);
		_fold_labels.resize(0);
		_references.resize(0);
		_dividers.resize(0);
		_rows.resize(0);
		_tooltip_owners.resize(0);
	}
}
