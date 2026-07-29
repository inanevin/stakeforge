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
#include "editor_project.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "editor_widget_checkbox.hpp"
#include "editor_widget_fold_label.hpp"
#include "editor_widget_input_field.hpp"
#include "editor_widget_reference.hpp"
#include "editor_widgets_color_field.hpp"
#include "editor_widgets_dropdown.hpp"
#include "editor_widgets_misc.hpp"
#include "editor_widgets_vec_fields.hpp"
#include "editor_widgets_dividers.hpp"
#include "editor_widgets_icons.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/scripting/script_runtime.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/engine_components.hpp>

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
				return sub_type_id == type_id_t<vec2f_t>::value || sub_type_id == type_id_t<vec2u16_t>::value || sub_type_id == type_id_t<vec3f_t>::value || sub_type_id == type_id_t<vec4f_t>::value || sub_type_id == type_id_t<quat_t>::value ||
					   sub_type_id == type_id_t<color_t>::value || (sub_type_id != 0 && reflection_registry_t::get().find_type(sub_type_id) != nullptr);
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
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;

		if (config.elevate_draw_order)
			tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		set_reflection(config);
	}

	void editor_widget_reflection_t::uninit()
	{
		_ui->cancel_mutations(this);
		clear_widgets();
		if (_blocker != NULL_WIDGET)
			_ui->deallocate_widget(_blocker);
		_ui->deallocate_widget(_root);

		_objects.resize(0);
		_callbacks				  = {};
		_field_callbacks		  = {};
		_fold_states			  = nullptr;
		_dropdown_items			  = nullptr;
		_dropdown_items_user_data = nullptr;
		_ui						  = nullptr;
		_type_id				  = 0;
		_world					  = {};
		_root					  = NULL_WIDGET;
		_blocker				  = NULL_WIDGET;
	}

	void editor_widget_reflection_t::set_reflection(const editor_widget_reflection_config_t& config)
	{
		_ui->cancel_mutations(this);
		clear_widgets();

		_fold_states			  = config.fold_states;
		_callbacks				  = config.callbacks;
		_dropdown_items			  = config.dropdown_items;
		_dropdown_items_user_data = config.dropdown_items_user_data;

		_field_callbacks = {
			.edit_begin		= on_field_edit_begin,
			.edited			= on_field_edited,
			.edit_submitted = on_field_edit_submitted,
			.user_data		= this,
		};

		_objects.resize(0);
		_objects.reserve(config.objects.size);

		for (size_t i = 0; i < config.objects.size; ++i)
			_objects.push_back(config.objects.data[i]);

		_type_id = config.type_id;
		_world	 = config.world;
		set_block_edits(config.block_edits);

		const reflected_type_t* type = reflection_registry_t::get().find_type(_type_id);
		if (type == nullptr)
			return;

		if (_objects.empty())
			return;

		editor_theme_t& theme = editor_theme_t::get();
		create_fields(_root, {.data = _objects.data(), .size = _objects.size()}, _type_id, _world, true, false, theme.margin_horizontal, true);
		refresh_dependency_visibility();
	}

	void editor_widget_reflection_t::set_block_edits(bool block_edits)
	{
		if (!block_edits)
		{
			if (_blocker != NULL_WIDGET)
			{
				_ui->deallocate_widget(_blocker);
				_blocker = NULL_WIDGET;
			}
			return;
		}

		if (_blocker == NULL_WIDGET)
			_blocker = editor_misc_widgets_t::add_edit_blocker(*_ui, _root);
	}

	bool editor_widget_reflection_t::is_field_visible(const reflected_type_t& type, const reflected_field_t& field, span_t<void*> objects, u32 dependency_depth) const
	{
		if (field.flags.is_set(reflected_field_flags_e::reflected_field_flag_no_ui))
			return false;

		if (field.ui_definition.dependency_field == 0)
			return true;

		if (dependency_depth >= type.fields.end - type.fields.start)
		{
			SFG_ASSERT(false);
			return false;
		}

		const reflected_field_t* dependency_field = nullptr;

		for (u32 i = type.fields.start; i < type.fields.end; ++i)
		{
			const reflected_field_t* candidate = reflection_registry_t::get().get_field(i);
			if (candidate->field_identifier == field.ui_definition.dependency_field)
			{
				dependency_field = candidate;
				break;
			}
		}

		SFG_ASSERT(dependency_field != nullptr);
		SFG_ASSERT(dependency_field->value_type == reflected_value_type_e::boolean || dependency_field->value_type == reflected_value_type_e::u8 || dependency_field->value_type == reflected_value_type_e::u16 ||
				   dependency_field->value_type == reflected_value_type_e::u32);
		SFG_ASSERT(dependency_field->size == sizeof(u8) || dependency_field->size == sizeof(u16) || dependency_field->size == sizeof(u32));

		if (!is_field_visible(type, *dependency_field, objects, dependency_depth + 1))
			return false;

		for (size_t i = 0; i < objects.size; ++i)
		{
			const u8* dependency_data  = static_cast<const u8*>(objects.data[i]) + dependency_field->offset;
			u32		  dependency_value = 0;

			switch (dependency_field->size)
			{
			case sizeof(u8):
				dependency_value = *reinterpret_cast<const u8*>(dependency_data);
				break;
			case sizeof(u16):
				dependency_value = *reinterpret_cast<const u16*>(dependency_data);
				break;
			case sizeof(u32):
				dependency_value = *reinterpret_cast<const u32*>(dependency_data);
				break;
			default:
				SFG_ASSERT(false);
				break;
			}

			switch (field.ui_definition.dependency_type)
			{
			case reflected_field_dependency_type_e::show_if_equals:
				if (dependency_value != field.ui_definition.dependency_value)
					return false;
				break;
			case reflected_field_dependency_type_e::show_if_not_equal:
				if (dependency_value == field.ui_definition.dependency_value)
					return false;
				break;
			}
		}

		return true;
	}

	void editor_widget_reflection_t::refresh_dependency_visibility()
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		for (dependent_field_t& dependent_field : _dependent_fields)
		{
			const bool visible = is_field_visible(*dependent_field.type, *dependent_field.field, {.data = dependent_field.objects.data(), .size = dependent_field.objects.size()}, 0);
			tree.set_visible(dependent_field.widget, visible);
		}

		for (const field_divider_t& field_divider : _field_dividers)
			tree.set_visible(field_divider.divider, false);

		for (const field_divider_t& field_divider : _field_dividers)
		{
			if ((tree.in_const(field_divider.widget).flags & ui::wf_visible) == 0)
				continue;

			ui::widget_id_t previous = tree.node(field_divider.divider).prev_sibling;
			while (previous != NULL_WIDGET && (tree.in_const(previous).flags & ui::wf_visible) == 0)
				previous = tree.node(previous).prev_sibling;

			tree.set_visible(field_divider.divider, previous != NULL_WIDGET);
		}
	}

	void editor_widget_reflection_t::create_fields(ui::widget_id_t parent, span_t<void*> objects, sid_t type_id, editor_world_handle_t world, bool track_rows, bool sub_item, f32 indentation, bool add_divider)
	{
		const reflected_type_t* type = reflection_registry_t::get().find_type(type_id);
		if (type == nullptr)
			return;

		const u32 field_start = type->fields.start;
		const u32 field_end	  = type->fields.end;
		bool	  has_fields  = false;
		for (u32 i = field_start; i < field_end; i++)
		{
			const reflected_field_t* field = reflection_registry_t::get().get_field(i);
			if (field == nullptr || field->flags.is_set(reflected_field_flags_e::reflected_field_flag_no_ui))
				continue;

			ui::widget_id_t divider = NULL_WIDGET;
			if (add_divider && has_fields)
			{
				divider = editor_dividers_t::add_divider_hor(*_ui, parent, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none);
				_dividers.push_back(divider);
			}
			has_fields = true;

			switch (field->value_type)
			{
			case reflected_value_type_e::boolean: {
				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_checkbox(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, false, indentation);
				break;
			}
			case reflected_value_type_e::string:
			case reflected_value_type_e::char_array: {
				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_input_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, false, indentation);
				break;
			}
			case reflected_value_type_e::object: {
				frame_vector_t<void*> object_fields;
				object_fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					object_fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_object(parent, type_id, field, {.data = object_fields.data(), .size = object_fields.size()}, world, track_rows, sub_item, false, indentation);
				break;
			}
			case reflected_value_type_e::container: {
				frame_vector_t<void*> container_fields;
				container_fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					container_fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_container(parent, type_id, field, {.data = container_fields.data(), .size = container_fields.size()}, world, track_rows, sub_item, indentation);
				break;
			}
			case reflected_value_type_e::bitmask: {
				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				create_bitmask_dropdown(parent, *type, *field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, false, indentation);
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

					if (create_reference(parent, field, {.data = reference_fields.data(), .size = reference_fields.size()}, world, track_rows, sub_item, false, indentation))
						break;
				}

				frame_vector_t<u8*> fields;
				fields.reserve(objects.size);
				for (size_t idx = 0; idx < objects.size; ++idx)
					fields.push_back(static_cast<u8*>(objects.data[idx]) + field->offset);

				if (create_dropdown(parent, field, field->field_identifier, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, false, indentation))
					break;

				create_input_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_rows, sub_item, false, indentation);
				break;
			}
			}

			const ui::widget_id_t field_widget = _ui->get_tree().node(parent).last_child;
			SFG_ASSERT(field_widget != NULL_WIDGET && field_widget != divider);

			if (divider != NULL_WIDGET)
				_field_dividers.push_back({.widget = field_widget, .divider = divider});

			if (field->ui_definition.dependency_field != 0)
			{
				dependent_field_t& dependent_field = _dependent_fields.emplace_back();
				dependent_field.type			   = type;
				dependent_field.field			   = field;
				dependent_field.widget			   = field_widget;
				dependent_field.objects.reserve(objects.size);

				for (size_t object_index = 0; object_index < objects.size; ++object_index)
					dependent_field.objects.push_back(objects.data[object_index]);
			}
		}
	}

	void editor_widget_reflection_t::fit_control(ui::widget_id_t widget)
	{
		ui::layout_tree_t& tree		= _ui->get_tree();
		ui::layout_in_t&   input_in = tree.in(widget);
		input_in.size_mode_x		= ui::axis_mode_e::fill;
		input_in.size_mode_y		= ui::axis_mode_e::fixed;
		input_in.pos_mode_y			= ui::pos_mode_e::relative_in_parent;
		input_in.anchor_y			= ui::anchor_e::center;
		input_in.pos_value.y		= 0.5f;
		input_in.size_value			= {1.0f, editor_theme_t::get().item_height};
	}

	void editor_widget_reflection_t::save_fold_states()
	{
		if (_fold_states == nullptr)
			return;

		for (const field_fold_t& field_fold : _field_folds)
			set_fold_state(field_fold.type_id, field_fold.field_id, field_fold.fold->is_folded());
	}

	void editor_widget_reflection_t::create_checkbox(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_checkbox_t* checkbox = new editor_checkbox_t();
		checkbox->init(*_ui,
					   row.right,
					   {
						   .field	  = {.fields = fields},
						   .callbacks = _field_callbacks,
					   });

		ui::layout_in_t& checkbox_in = _ui->get_tree().in(checkbox->get_root());
		checkbox_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		checkbox_in.anchor_y		 = ui::anchor_e::center;
		checkbox_in.pos_value.y		 = 0.5f;
		if (removable_item)
			install_sub_item_button(row.right, NULL_WIDGET, container_data, element_index);

		_checkboxes.push_back(checkbox);
		if (track_row)
			_rows.push_back(row.row);
	}

	bool editor_widget_reflection_t::create_dropdown(
		ui::widget_id_t parent, const reflected_field_t* field, sid_t owner_field_id, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		if (field->value_type != reflected_value_type_e::u8 && field->value_type != reflected_value_type_e::u16 && field->value_type != reflected_value_type_e::u32 && field->value_type != reflected_value_type_e::u64)
			return false;

		frame_vector_t<editor_dropdown_item_t> items = {};

		if (_dropdown_items != nullptr)
		{
			const span_t<const editor_widget_reflection_dropdown_item_t> resolved_items = _dropdown_items(field->field_identifier, owner_field_id, _dropdown_items_user_data);

			items.reserve(resolved_items.size);

			for (size_t item_index = 0; item_index < resolved_items.size; ++item_index)
				items.push_back({.text = resolved_items.data[item_index].text, .value = resolved_items.data[item_index].value});
		}

		if (items.empty())
		{
			if (field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_WORLD_SCRIPT)
			{
				const vector_t<script_world_script_desc_t>& world_scripts = script_runtime_t::get().get_component_schema().get_world_scripts();
				SFG_ASSERT(world_scripts.size() + 1 <= editor_popup_controller_t::MAX_ITEMS);

				items.reserve(world_scripts.size() + 1);
				items.push_back({.text = "None", .value = NULL_SID});

				for (const script_world_script_desc_t& world_script : world_scripts)
					items.push_back({.text = world_script.full_name.c_str(), .value = world_script.type_id});
			}
			else if (field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_COLLISION_LAYER)
			{
				const vector_t<physics_collision_layer_definition_t>& layers = editor_project_t::get().settings.project_settings.physics.collision_layers;
				items.reserve(layers.size());

				for (u32 i = 0; i < layers.size(); ++i)
					items.push_back({.text = layers[i].name.empty() ? "Unnamed Layer" : layers[i].name.c_str(), .value = layers[i].slot});
			}
			else
			{
				const reflected_type_t* enum_type = reflection_registry_t::get().find_type(field->sub_type_id);

				if (enum_type == nullptr || !enum_type->flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
					return false;

				const u32 enum_item_count = enum_type->fields.end - enum_type->fields.start;
				items.reserve(enum_item_count);

				for (u32 i = 0; i < enum_item_count; ++i)
				{
					const reflected_field_t* enum_field = reflection_registry_t::get().get_field(enum_type->fields.start + i);

					items.push_back({.text = enum_field->display_name != nullptr ? enum_field->display_name : enum_field->name, .value = static_cast<u16>(i)});
				}
			}
		}

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_dropdown_t* dropdown = new editor_dropdown_t();
		dropdown->init(*_ui,
					   row.right,
					   {
						   .items	   = items.data(),
						   .field	   = {.fields = fields, .field_size = field->size},
						   .callbacks  = _field_callbacks,
						   .item_count = static_cast<u16>(items.size()),
						   .width	   = editor_dropdown_width_e::parent_relative,
						   .pos_y	   = editor_dropdown_pos_y_e::center,
					   });

		fit_control(dropdown->get_root());

		if (removable_item)
			install_sub_item_button(row.right, dropdown->get_root(), container_data, element_index);

		_dropdowns.push_back(dropdown);

		if (track_row)
			_rows.push_back(row.row);

		return true;
	}

	void editor_widget_reflection_t::create_bitmask_dropdown(
		ui::widget_id_t parent, const reflected_type_t& type, const reflected_field_t& field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		SFG_ASSERT(type.bitmask_opts.get_option_count_fn != nullptr && type.bitmask_opts.get_option_fn != nullptr && type.bitmask_opts.build_title_fn != nullptr);
		const u32 option_count = type.bitmask_opts.get_option_count_fn(type.bitmask_opts.user_data);
		SFG_ASSERT(option_count + 1 <= editor_popup_controller_t::MAX_ITEMS);

		frame_vector_t<editor_dropdown_item_t> items;
		items.reserve(option_count + 1);
		items.push_back({.text = "None", .value = 0});
		for (u32 i = 0; i < option_count; ++i)
		{
			const bitmask_option_t option = type.bitmask_opts.get_option_fn(i, type.bitmask_opts.user_data);
			items.push_back({.text = option.name, .value = option.value});
		}

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field.display_name ? field.display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field.tooltip);

		editor_dropdown_t* dropdown = new editor_dropdown_t();
		dropdown->init(*_ui,
					   row.right,
					   {
						   .items			= items.data(),
						   .build_title		= type.bitmask_opts.build_title_fn,
						   .title_user_data = type.bitmask_opts.user_data,
						   .field			= {.fields = fields, .field_size = field.size},
						   .callbacks		= _field_callbacks,
						   .item_count		= static_cast<u16>(items.size()),
						   .width			= editor_dropdown_width_e::parent_relative,
						   .pos_y			= editor_dropdown_pos_y_e::center,
						   .is_bitmask		= true,
					   });
		fit_control(dropdown->get_root());
		if (removable_item)
			install_sub_item_button(row.right, dropdown->get_root(), container_data, element_index);
		_dropdowns.push_back(dropdown);
		if (track_row)
			_rows.push_back(row.row);
	}

	void editor_widget_reflection_t::create_input_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
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
		const f32  increment	 = is_slider ? field->ui_definition.clamp_granularity : (integer ? 1.0f : 0.1f);
		const f32  min_value	 = is_slider ? field->ui_definition.min_clamp : (signed_number ? -1.0f : 0.0f);
		const f32  max_value	 = is_slider ? field->ui_definition.max_clamp : 1.0f;

		editor_input_field_t* input = new editor_input_field_t();
		input->init(*_ui,
					row.right,
					{
						.field =
							{
								.fields		= fields,
								.field_size = field->size,
								.type		= input_type,
								.is_slider	= is_slider,
							},
						.callbacks	 = _field_callbacks,
						.placeholder = field->name,
						.increment	 = increment,
						.min_value	 = min_value,
						.max_value	 = max_value,
						.is_integer	 = integer,
					});

		fit_control(input->get_root());

		if (field->value_type == reflected_value_type_e::string && (field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_DIRECTORY || field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_PATH))
			install_path_picker_button(row.right, input, field->sub_type_id);

		if (removable_item)
			install_sub_item_button(row.right, input->get_root(), container_data, element_index);

		_inputs.push_back(input);
		if (track_row)
			_rows.push_back(row.row);
	}

	bool editor_widget_reflection_t::create_reference(
		ui::widget_id_t parent, const reflected_field_t* field, span_t<u64*> fields, editor_world_handle_t world, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		const editor_asset_type_e asset_type			 = editor_asset_type_from_reflection_sub_type_id(field->sub_type_id);
		const bool				  any_resource_reference = field->sub_type_id == SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_ANY_RESOURCE;
		const bool				  entity_reference		 = field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID;

		if (!entity_reference && !any_resource_reference && asset_type == editor_asset_type_e::invalid)
			return false;

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		editor_widget_reference_t* reference = new editor_widget_reference_t();
		reference->init(*_ui,
						row.right,
						{
							.callbacks				 = _field_callbacks,
							.fields					 = fields,
							.world					 = world,
							.asset_type				 = asset_type,
							.type					 = entity_reference ? editor_widget_reference_type_e::entity : editor_widget_reference_type_e::asset,
							.allow_any_resource_type = any_resource_reference,
						});
		fit_control(reference->get_root());
		if (removable_item)
			install_sub_item_button(row.right, reference->get_root(), container_data, element_index);
		_references.push_back(reference);
		if (track_row)
			_rows.push_back(row.row);
		return true;
	}

	bool editor_widget_reflection_t::create_quat_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		if (field->sub_type_id != type_id_t<quat_t>::value)
			return false;

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		frame_vector_t<quat_t*> quat_fields;
		quat_fields.reserve(fields.size);
		for (size_t i = 0; i < fields.size; ++i)
			quat_fields.push_back(reinterpret_cast<quat_t*>(fields.data[i]));

		editor_quat_field_t* quat = new editor_quat_field_t();
		quat->init(*_ui, row.right, {.field = {.fields = {.data = quat_fields.data(), .size = quat_fields.size()}}, .callbacks = _field_callbacks});
		fit_control(quat->get_root());
		if (removable_item)
			install_sub_item_button(row.right, quat->get_root(), container_data, element_index);
		_quat_fields.push_back(quat);
		if (track_row)
			_rows.push_back(row.row);
		return true;
	}

	bool editor_widget_reflection_t::create_vector_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
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
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}, .callbacks = _field_callbacks});
			fit_control(vec->get_root());
			if (removable_item)
				install_sub_item_button(row.right, vec->get_root(), container_data, element_index);
			_vec2_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}

		if (field->sub_type_id == type_id_t<vec2u16_t>::value)
		{
			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
			install_tooltip(row.label, field->tooltip);

			frame_vector_t<vec2u16_t*> vector_fields;
			vector_fields.reserve(fields.size);

			for (size_t i = 0; i < fields.size; ++i)
				vector_fields.push_back(reinterpret_cast<vec2u16_t*>(fields.data[i]));

			editor_vec2u16_field_t* vec = new editor_vec2u16_field_t();
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}, .callbacks = _field_callbacks});
			fit_control(vec->get_root());

			if (removable_item)
				install_sub_item_button(row.right, vec->get_root(), container_data, element_index);

			_vec2u16_fields.push_back(vec);

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
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}, .callbacks = _field_callbacks});
			fit_control(vec->get_root());
			if (removable_item)
				install_sub_item_button(row.right, vec->get_root(), container_data, element_index);
			_vec3_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}
		if (field->sub_type_id == type_id_t<vec4f_t>::value)
		{
			const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
			install_tooltip(row.label, field->tooltip);

			frame_vector_t<vec4f_t*> vector_fields;
			vector_fields.reserve(fields.size);
			for (size_t i = 0; i < fields.size; ++i)
				vector_fields.push_back(reinterpret_cast<vec4f_t*>(fields.data[i]));

			editor_vec4_field_t* vec = new editor_vec4_field_t();
			vec->init(*_ui, row.right, {.field = {.fields = {.data = vector_fields.data(), .size = vector_fields.size()}}, .callbacks = _field_callbacks});
			fit_control(vec->get_root());
			if (removable_item)
				install_sub_item_button(row.right, vec->get_root(), container_data, element_index);
			_vec4_fields.push_back(vec);
			if (track_row)
				_rows.push_back(row.row);
			return true;
		}

		return false;
	}

	bool editor_widget_reflection_t::create_color_field(ui::widget_id_t parent, const reflected_field_t* field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		if (field->sub_type_id != type_id_t<color_t>::value)
			return false;

		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, field->display_name ? field->display_name : "missing_display_name", sub_item, false, indentation);
		install_tooltip(row.label, field->tooltip);

		frame_vector_t<color_t*> color_fields;
		color_fields.reserve(fields.size);
		for (size_t i = 0; i < fields.size; ++i)
			color_fields.push_back(reinterpret_cast<color_t*>(fields.data[i]));

		editor_color_field_t* color = new editor_color_field_t();
		color->init(*_ui, row.right, {.field = {.fields = {.data = color_fields.data(), .size = color_fields.size()}}, .callbacks = _field_callbacks});
		fit_control(color->get_root());
		if (removable_item)
			install_sub_item_button(row.right, color->get_root(), container_data, element_index);
		_color_fields.push_back(color);
		if (track_row)
			_rows.push_back(row.row);
		return true;
	}

	void editor_widget_reflection_t::create_object(
		ui::widget_id_t parent, sid_t type_id, const reflected_field_t* field, span_t<void*> objects, editor_world_handle_t world, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data, u32 element_index)
	{
		frame_vector_t<u8*> fields;
		fields.reserve(objects.size);
		for (size_t i = 0; i < objects.size; ++i)
			fields.push_back(static_cast<u8*>(objects.data[i]));

		if (create_vector_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_row, sub_item, removable_item, indentation, container_data, element_index))
			return;

		if (create_quat_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_row, sub_item, removable_item, indentation, container_data, element_index))
			return;

		if (create_color_field(parent, field, {.data = fields.data(), .size = fields.size()}, track_row, sub_item, removable_item, indentation, container_data, element_index))
			return;

		bool folded = false;
		get_fold_state(type_id, field->field_identifier, folded);

		editor_widget_fold_label_t* fold = new editor_widget_fold_label_t();
		fold->init(*_ui,
				   parent,
				   {
					   .label		 = field->display_name ? field->display_name : "missing_display_name",
					   .indentation	 = indentation,
					   .button_style = removable_item ? editor_widget_fold_label_button_style_e::container_item_buttons : editor_widget_fold_label_button_style_e::none,
					   .folded		 = folded,
					   .sub_item	 = sub_item,
				   });
		install_tooltip(fold->get_root(), field->tooltip);
		if (removable_item)
		{
			install_tooltip(fold->get_remove_button(), "Remove Element");
			if (container_data != nullptr)
				install_container_element_remove_listener(fold->get_remove_button(), container_data, element_index);
		}
		_fold_labels.push_back(fold);
		_field_folds.push_back({.fold = fold, .type_id = type_id, .field_id = field->field_identifier});
		create_fields(fold->get_body(), objects, field->sub_type_id, world, false, true, indentation + editor_theme_t::get().margin_horizontal, false);
	}

	void editor_widget_reflection_t::create_container(ui::widget_id_t parent, sid_t type_id, const reflected_field_t* field, span_t<void*> containers, editor_world_handle_t world, bool, bool sub_item, f32 indentation)
	{
		const reflected_value_type_e element_value_type	 = field->container_ops.element_value_type;
		const sid_t					 element_sub_type_id = field->container_ops.element_sub_type_id;
		SFG_ASSERT(is_container_type_allowed(element_value_type, element_sub_type_id));

		bool folded = false;
		get_fold_state(type_id, field->field_identifier, folded);

		editor_widget_fold_label_t* fold = new editor_widget_fold_label_t();
		fold->init(*_ui,
				   parent,
				   {
					   .label		 = field->display_name ? field->display_name : "missing_display_name",
					   .indentation	 = indentation,
					   .button_style = editor_widget_fold_label_button_style_e::container_buttons,
					   .folded		 = folded,
					   .sub_item	 = sub_item,
				   });
		install_tooltip(fold->get_root(), field->tooltip);
		install_tooltip(fold->get_add_button(), "Add Element");
		install_tooltip(fold->get_reset_button(), "Reset Container");
		container_user_data_t* user_data = create_container_user_data(field, containers, world, type_id, indentation, fold);

		ui::listener_bundle_t add_listener = {};
		add_listener.user_data			   = user_data;
		add_listener.on_click			   = on_container_add;
		_ui->get_input().set_listener(fold->get_add_button(), add_listener);

		ui::listener_bundle_t reset_listener = {};
		reset_listener.user_data			 = user_data;
		reset_listener.on_click				 = on_container_reset;
		_ui->get_input().set_listener(fold->get_reset_button(), reset_listener);
		_fold_labels.push_back(fold);
		_field_folds.push_back({.fold = fold, .type_id = type_id, .field_id = field->field_identifier});

		create_container_elements(fold->get_body(), user_data);
	}

	void editor_widget_reflection_t::create_container_elements(ui::widget_id_t parent, container_user_data_t* container_data)
	{
		const reflected_field_t* const field			   = container_data->field;
		span_t<void*>				   containers		   = {.data = container_data->containers.data(), .size = container_data->containers.size()};
		const editor_world_handle_t	   world			   = container_data->world;
		const f32					   indentation		   = container_data->indentation;
		const reflected_value_type_e   element_value_type  = field->container_ops.element_value_type;
		const sid_t					   element_sub_type_id = field->container_ops.element_sub_type_id;

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
				.name			  = "Element",
				.display_name	  = element_label,
				.field_identifier = hashing_t::hash_u64_combine(field->field_identifier, element_index),
				.sub_type_id	  = element_sub_type_id,
				.size			  = field->container_ops.element_value_size,
				.value_type		  = element_value_type,
			};

			switch (element_value_type)
			{
			case reflected_value_type_e::boolean: {
				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_checkbox(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index);
				break;
			}
			case reflected_value_type_e::string:
			case reflected_value_type_e::char_array: {
				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_input_field(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index);
				break;
			}
			case reflected_value_type_e::object: {
				frame_vector_t<void*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				create_object(parent, container_data->type_id, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, world, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index);
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

					if (create_reference(parent, &element_field, {.data = reference_fields.data(), .size = reference_fields.size()}, world, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index))
						break;
				}

				frame_vector_t<u8*> element_fields;
				element_fields.reserve(containers.size);
				for (size_t i = 0; i < containers.size; ++i)
					element_fields.push_back(field->container_ops.get_element_ptr_fn(containers.data[i], element_index));

				if (create_dropdown(parent, &element_field, field->field_identifier, {.data = element_fields.data(), .size = element_fields.size()}, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index))
					break;

				create_input_field(parent, &element_field, {.data = element_fields.data(), .size = element_fields.size()}, false, true, true, indentation + editor_theme_t::get().margin_horizontal, container_data, element_index);
				break;
			}
			default:
				SFG_ASSERT(false);
				break;
			}
		}
	}

	editor_widget_reflection_t::container_user_data_t* editor_widget_reflection_t::create_container_user_data(
		const reflected_field_t* field, span_t<void*> containers, editor_world_handle_t world, sid_t type_id, f32 indentation, editor_widget_fold_label_t* fold)
	{
		container_user_data_t* data = new container_user_data_t();
		data->field					= field;
		data->reflection			= this;
		data->fold					= fold;
		data->world					= world;
		data->type_id				= type_id;
		data->indentation			= indentation;
		data->containers.reserve(containers.size);
		for (size_t i = 0; i < containers.size; ++i)
			data->containers.push_back(containers.data[i]);
		_container_user_data.push_back(data);
		return data;
	}

	bool editor_widget_reflection_t::get_fold_state(sid_t type_id, sid_t field_id, bool& out_folded) const
	{
		if (_fold_states == nullptr)
			return false;

		for (const editor_widget_reflection_fold_state_t& state : *_fold_states)
		{
			if (state.type_id == type_id && state.field_id == field_id)
			{
				out_folded = state.folded;
				return true;
			}
		}
		return false;
	}

	void editor_widget_reflection_t::set_fold_state(sid_t type_id, sid_t field_id, bool folded)
	{
		for (editor_widget_reflection_fold_state_t& state : *_fold_states)
		{
			if (state.type_id == type_id && state.field_id == field_id)
			{
				state.folded = folded;
				return;
			}
		}
		_fold_states->push_back({.type_id = type_id, .field_id = field_id, .folded = folded});
	}

	editor_widget_reflection_t::container_element_user_data_t* editor_widget_reflection_t::create_container_element_user_data(container_user_data_t* container_data, u32 element_index, ui::widget_id_t button)
	{
		container_element_user_data_t* data = new container_element_user_data_t();
		data->container_data				= container_data;
		data->button						= button;
		data->element_index					= element_index;
		_container_element_user_data.push_back(data);
		return data;
	}

	void editor_widget_reflection_t::request_container_refresh(container_user_data_t& data)
	{
		_ui->request_unique_mutation(on_container_refresh, &data);
	}

	void editor_widget_reflection_t::refresh_container(container_user_data_t& data)
	{
		clear_container_widgets(data.fold->get_body());
		data.fold->clear_children();
		create_container_elements(data.fold->get_body(), &data);
		refresh_dependency_visibility();
	}

	bool editor_widget_reflection_t::is_child_widget(ui::widget_id_t widget, ui::widget_id_t parent) const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		if (!tree.is_alive(widget))
			return false;

		ui::widget_id_t cursor = widget;
		while (cursor != NULL_WIDGET)
		{
			if (cursor == parent)
				return true;
			cursor = tree.node(cursor).parent;
		}
		return false;
	}

	void editor_widget_reflection_t::clear_child_tooltips(ui::widget_id_t parent)
	{
		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		for (size_t i = 0; i < _tooltip_owners.size();)
		{
			if (is_child_widget(_tooltip_owners[i], parent))
			{
				tooltip_controller->clear_tooltip(_tooltip_owners[i]);
				_tooltip_owners.erase(_tooltip_owners.begin() + i);
				continue;
			}
			++i;
		}
	}

	void editor_widget_reflection_t::clear_container_widgets(ui::widget_id_t parent)
	{
		clear_child_tooltips(parent);

		for (size_t i = 0; i < _dependent_fields.size();)
		{
			if (is_child_widget(_dependent_fields[i].widget, parent))
			{
				_dependent_fields.erase(_dependent_fields.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _field_dividers.size();)
		{
			if (is_child_widget(_field_dividers[i].widget, parent))
			{
				_field_dividers.erase(_field_dividers.begin() + i);
				continue;
			}
			++i;
		}

		for (size_t i = 0; i < _container_element_user_data.size();)
		{
			if (is_child_widget(_container_element_user_data[i]->button, parent))
			{
				delete _container_element_user_data[i];
				_container_element_user_data.erase(_container_element_user_data.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _path_picker_user_data.size();)
		{
			if (is_child_widget(_path_picker_user_data[i]->button, parent))
			{
				delete _path_picker_user_data[i];
				_path_picker_user_data.erase(_path_picker_user_data.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _inputs.size();)
		{
			if (is_child_widget(_inputs[i]->get_root(), parent))
			{
				_inputs[i]->uninit();
				delete _inputs[i];
				_inputs.erase(_inputs.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _checkboxes.size();)
		{
			if (is_child_widget(_checkboxes[i]->get_root(), parent))
			{
				_checkboxes[i]->uninit();
				delete _checkboxes[i];
				_checkboxes.erase(_checkboxes.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _dropdowns.size();)
		{
			if (is_child_widget(_dropdowns[i]->get_root(), parent))
			{
				_dropdowns[i]->uninit();
				delete _dropdowns[i];
				_dropdowns.erase(_dropdowns.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _vec2_fields.size();)
		{
			if (is_child_widget(_vec2_fields[i]->get_root(), parent))
			{
				_vec2_fields[i]->uninit();
				delete _vec2_fields[i];
				_vec2_fields.erase(_vec2_fields.begin() + i);
				continue;
			}
			++i;
		}

		for (size_t i = 0; i < _vec2u16_fields.size();)
		{
			if (is_child_widget(_vec2u16_fields[i]->get_root(), parent))
			{
				_vec2u16_fields[i]->uninit();
				delete _vec2u16_fields[i];
				_vec2u16_fields.erase(_vec2u16_fields.begin() + i);
				continue;
			}

			++i;
		}

		for (size_t i = 0; i < _quat_fields.size();)
		{
			if (is_child_widget(_quat_fields[i]->get_root(), parent))
			{
				_quat_fields[i]->uninit();
				delete _quat_fields[i];
				_quat_fields.erase(_quat_fields.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _vec3_fields.size();)
		{
			if (is_child_widget(_vec3_fields[i]->get_root(), parent))
			{
				_vec3_fields[i]->uninit();
				delete _vec3_fields[i];
				_vec3_fields.erase(_vec3_fields.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _vec4_fields.size();)
		{
			if (is_child_widget(_vec4_fields[i]->get_root(), parent))
			{
				_vec4_fields[i]->uninit();
				delete _vec4_fields[i];
				_vec4_fields.erase(_vec4_fields.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _color_fields.size();)
		{
			if (is_child_widget(_color_fields[i]->get_root(), parent))
			{
				_color_fields[i]->uninit();
				delete _color_fields[i];
				_color_fields.erase(_color_fields.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _references.size();)
		{
			if (is_child_widget(_references[i]->get_root(), parent))
			{
				_references[i]->uninit();
				delete _references[i];
				_references.erase(_references.begin() + i);
				continue;
			}
			++i;
		}
		for (size_t i = 0; i < _fold_labels.size();)
		{
			if (is_child_widget(_fold_labels[i]->get_root(), parent))
			{
				for (size_t field_fold_idx = 0; field_fold_idx < _field_folds.size();)
				{
					if (_field_folds[field_fold_idx].fold == _fold_labels[i])
					{
						_field_folds.erase(_field_folds.begin() + field_fold_idx);
						continue;
					}
					++field_fold_idx;
				}
				_fold_labels[i]->uninit();
				delete _fold_labels[i];
				_fold_labels.erase(_fold_labels.begin() + i);
				continue;
			}
			++i;
		}
	}

	ui::widget_id_t editor_widget_reflection_t::install_sub_item_button(ui::widget_id_t parent, ui::widget_id_t control, container_user_data_t* container_data, u32 element_index)
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
			filler_in.size_mode_x	   = ui::axis_mode_e::fill;
			filler_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			filler_in.size_value	   = {1.0f, 1.0f};
		}

		const ui::widget_id_t remove_button = editor_icon_widgets_t::add_naked_icon_button(*_ui, parent, ICON_CROSS, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
		install_tooltip(remove_button, "Remove Element");
		if (container_data != nullptr)
			install_container_element_remove_listener(remove_button, container_data, element_index);
		return remove_button;
	}

	void editor_widget_reflection_t::install_container_element_remove_listener(ui::widget_id_t button, container_user_data_t* container_data, u32 element_index)
	{
		ui::listener_bundle_t remove_listener = {};
		remove_listener.user_data			  = create_container_element_user_data(container_data, element_index, button);
		remove_listener.on_click			  = on_container_element_remove;
		_ui->get_input().set_listener(button, remove_listener);
	}

	void editor_widget_reflection_t::install_path_picker_button(ui::widget_id_t parent, editor_input_field_t* input, sid_t sub_type_id)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& input_in = tree.in(input->get_root());
		input_in.size_mode_x	  = ui::axis_mode_e::fill;
		input_in.size_value.x	  = 1.0f;

		const ui::widget_id_t button = editor_icon_widgets_t::add_naked_icon_button(*_ui, parent, ICON_FOLDER, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
		install_tooltip(button, sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_DIRECTORY ? "Select Directory" : "Select File");

		path_picker_user_data_t* data = new path_picker_user_data_t();
		data->input					  = input;
		data->sub_type_id			  = sub_type_id;
		data->button				  = button;
		_path_picker_user_data.push_back(data);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = data;
		listener.on_click			   = on_path_picker;
		_ui->get_input().set_listener(button, listener);
	}

	void editor_widget_reflection_t::on_container_add(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		container_user_data_t& data = *static_cast<container_user_data_t*>(user_data);
		if (data.reflection->_callbacks.edit_begin != nullptr)
			data.reflection->_callbacks.edit_begin(data.reflection->_callbacks.user_data);
		for (void* container : data.containers)
			data.field->container_ops.add_element_ptr_fn(container);
		if (data.reflection->_callbacks.edited != nullptr)
			data.reflection->_callbacks.edited(data.reflection->_callbacks.user_data);
		data.reflection->request_container_refresh(data);
		if (data.reflection->_callbacks.edit_submitted != nullptr)
			data.reflection->_callbacks.edit_submitted(data.reflection->_callbacks.user_data);
	}

	void editor_widget_reflection_t::on_container_reset(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		container_user_data_t& data = *static_cast<container_user_data_t*>(user_data);
		if (data.reflection->_callbacks.edit_begin != nullptr)
			data.reflection->_callbacks.edit_begin(data.reflection->_callbacks.user_data);
		for (void* container : data.containers)
			data.field->container_ops.reset_fn(container);
		if (data.reflection->_callbacks.edited != nullptr)
			data.reflection->_callbacks.edited(data.reflection->_callbacks.user_data);
		data.reflection->request_container_refresh(data);
		if (data.reflection->_callbacks.edit_submitted != nullptr)
			data.reflection->_callbacks.edit_submitted(data.reflection->_callbacks.user_data);
	}

	void editor_widget_reflection_t::on_container_element_remove(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		container_element_user_data_t& data			  = *static_cast<container_element_user_data_t*>(user_data);
		container_user_data_t&		   container_data = *data.container_data;
		if (container_data.reflection->_callbacks.edit_begin != nullptr)
			container_data.reflection->_callbacks.edit_begin(container_data.reflection->_callbacks.user_data);
		for (void* container : container_data.containers)
			container_data.field->container_ops.remove_index_fn(container, data.element_index);
		if (container_data.reflection->_callbacks.edited != nullptr)
			container_data.reflection->_callbacks.edited(container_data.reflection->_callbacks.user_data);
		container_data.reflection->request_container_refresh(container_data);
		if (container_data.reflection->_callbacks.edit_submitted != nullptr)
			container_data.reflection->_callbacks.edit_submitted(container_data.reflection->_callbacks.user_data);
	}

	void editor_widget_reflection_t::on_path_picker(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		path_picker_user_data_t& data = *static_cast<path_picker_user_data_t*>(user_data);
		const string_t			 path = data.sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_DIRECTORY ? process::select_folder("Select Directory") : process::select_file("Select File", "");
		if (path.empty())
			return;

		data.input->set_text(path.c_str());
	}

	void editor_widget_reflection_t::on_container_refresh(ui::ui_context&, void* user_data)
	{
		container_user_data_t& data = *static_cast<container_user_data_t*>(user_data);
		data.reflection->refresh_container(data);
	}

	void editor_widget_reflection_t::on_field_edit_begin(void* user_data)
	{
		editor_widget_reflection_t& reflection = *static_cast<editor_widget_reflection_t*>(user_data);
		if (reflection._callbacks.edit_begin != nullptr)
			reflection._callbacks.edit_begin(reflection._callbacks.user_data);
	}

	void editor_widget_reflection_t::on_field_edited(void* user_data)
	{
		editor_widget_reflection_t& reflection = *static_cast<editor_widget_reflection_t*>(user_data);
		if (reflection._callbacks.edited != nullptr)
			reflection._callbacks.edited(reflection._callbacks.user_data);
	}

	void editor_widget_reflection_t::on_field_edit_submitted(void* user_data)
	{
		editor_widget_reflection_t& reflection = *static_cast<editor_widget_reflection_t*>(user_data);
		if (reflection._callbacks.edit_submitted != nullptr)
			reflection._callbacks.edit_submitted(reflection._callbacks.user_data);

		if (!reflection._dependent_fields.empty())
			reflection.refresh_dependency_visibility();
	}

	void editor_widget_reflection_t::install_tooltip(ui::widget_id_t owner, const char* text)
	{
		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		editor_tooltip_desc_t		 tooltip			= {};
		tooltip.text									= text;
		tooltip_controller->set_tooltip(owner, tooltip);

		_tooltip_owners.push_back(owner);
	}

	void editor_widget_reflection_t::clear_tooltips()
	{
		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		for (ui::widget_id_t owner : _tooltip_owners)
			tooltip_controller->clear_tooltip(owner);
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
		for (editor_dropdown_t* dropdown : _dropdowns)
		{
			dropdown->uninit();
			delete dropdown;
		}
		for (editor_color_field_t* color : _color_fields)
		{
			color->uninit();
			delete color;
		}
		for (editor_quat_field_t* quat : _quat_fields)
		{
			quat->uninit();
			delete quat;
		}
		for (editor_vec2_field_t* vec : _vec2_fields)
		{
			vec->uninit();
			delete vec;
		}

		for (editor_vec2u16_field_t* vec : _vec2u16_fields)
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

		for (container_user_data_t* data : _container_user_data)
		{
			_ui->cancel_mutations(data);
			delete data;
		}
		for (container_element_user_data_t* data : _container_element_user_data)
			delete data;
		for (path_picker_user_data_t* data : _path_picker_user_data)
			delete data;

		_inputs.resize(0);
		_checkboxes.resize(0);
		_dropdowns.resize(0);
		_color_fields.resize(0);
		_quat_fields.resize(0);
		_vec2_fields.resize(0);
		_vec2u16_fields.resize(0);
		_vec3_fields.resize(0);
		_vec4_fields.resize(0);
		_fold_labels.resize(0);
		_references.resize(0);
		_container_user_data.resize(0);
		_container_element_user_data.resize(0);
		_path_picker_user_data.resize(0);
		_field_folds.resize(0);
		_dependent_fields.resize(0);
		_field_dividers.resize(0);
		_dividers.resize(0);
		_rows.resize(0);
		_tooltip_owners.resize(0);
	}
}
