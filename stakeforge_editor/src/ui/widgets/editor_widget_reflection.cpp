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
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
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
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		set_reflection(config);
	}

	void editor_widget_reflection_t::uninit()
	{
		clear_widgets();
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_widget_reflection_t::set_reflection(const editor_widget_reflection_config_t& ab)
	{
		clear_widgets();

		const reflected_type_t* type = reflection_registry_t::get().find_type(ab.type_id);
		if (type == nullptr)
			return;

		if (ab.objects.size == 0)
			return;

		const u32 field_start = type->fields.start;
		const u32 field_end	  = type->fields.end;
		for (u32 i = field_start; i < field_end; i++)
		{
			const reflected_field_t* field = reflection_registry_t::get().get_field(i);
			if (field == nullptr || field->flags.is_set(reflected_field_flags_e::reflected_field_flag_no_ui))
				continue;

			/*
					f32,
		u64,
		i64,
		u32,
		i32,
		u16,
		i16,
		u8,
		i8,
		boolean,
		string,
		object,
		container,
		char_array,
			*/
			switch (field->value_type)
			{
			case reflected_value_type_e::f32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::i32:
			case reflected_value_type_e::u16:
			case reflected_value_type_e::i16:
			case reflected_value_type_e::u8:
			case reflected_value_type_e::i8:
			case reflected_value_type_e::u64:
			case reflected_value_type_e::i64: {

				const editor_property_row_t row				 = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, field->display_name ? field->display_name : "missing_display_name");
				const resource_type_e		resource_type	 = field->value_type == reflected_value_type_e::u64 ? resource_type_from_reflection_sub_type_id(field->sub_type_id) : resource_type_e::invalid;
				const editor_asset_type_e	asset_type		 = editor_asset_type_from_resource_type(resource_type);
				const bool					entity_reference = field->value_type == reflected_value_type_e::u64 && field->sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID;
				if (entity_reference || asset_type != editor_asset_type_e::invalid)
				{
					frame_vector_t<u64*> fields;
					fields.reserve(ab.objects.size);
					for (size_t idx = 0; idx < ab.objects.size; ++idx)
						fields.push_back(reinterpret_cast<u64*>(static_cast<u8*>(ab.objects.data[idx]) + field->offset));

					editor_widget_reference_t* reference = new editor_widget_reference_t();
					reference->init(*_ui,
									row.right,
									{
										.fields		= {.data = fields.data(), .size = fields.size()},
										.world		= ab.world,
										.asset_type = asset_type,
										.type		= entity_reference ? editor_widget_reference_type_e::entity : editor_widget_reference_type_e::asset,
									});
					ui::layout_in_t& reference_in = _ui->get_tree().in(reference->get_root());
					reference_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
					reference_in.size_mode_y	  = ui::axis_mode_e::fixed;
					reference_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
					reference_in.anchor_y		  = ui::anchor_e::center;
					reference_in.pos_value.y	  = 0.5f;
					reference_in.size_value		  = {1.0f, editor_theme_t::get().item_height};
					_references.push_back(reference);
					_rows.push_back(row.row);
					break;
				}

				frame_vector_t<u8*> fields;
				fields.reserve(ab.objects.size);
				for (size_t idx = 0; idx < ab.objects.size; ++idx)
					fields.push_back(static_cast<u8*>(ab.objects.data[idx]) + field->offset);

				editor_input_field_t* input			= new editor_input_field_t();
				const bool			  integer		= field->value_type != reflected_value_type_e::f32;
				const bool			  signed_number = field->value_type == reflected_value_type_e::i64 || field->value_type == reflected_value_type_e::i32 || field->value_type == reflected_value_type_e::i16 || field->value_type == reflected_value_type_e::i8;
				input->init(*_ui,
							row.right,
							{
								.placeholder = field->name,
								.field =
									{
										.type		= editor_input_field_field_type_e::pod_number,
										.fields		= {.data = fields.data(), .size = fields.size()},
										.field_size = field->size,
									},
								.increment = integer ? 1.0f : 0.1f,
								.min_value = signed_number ? -1.0f : 0.0f,
								.max_value = 1.0f,
							});
				ui::layout_in_t& input_in = _ui->get_tree().in(input->get_root());
				input_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
				input_in.size_mode_y	  = ui::axis_mode_e::fixed;
				input_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				input_in.anchor_y		  = ui::anchor_e::center;
				input_in.pos_value.y	  = 0.5f;
				input_in.size_value		  = {1.0f, editor_theme_t::get().item_height};
				_inputs.push_back(input);
				_rows.push_back(row.row);
				break;
			}
			}
		}
	}

	void editor_widget_reflection_t::clear_widgets()
	{
		for (editor_input_field_t* input : _inputs)
		{
			input->uninit();
			delete input;
		}
		for (editor_widget_reference_t* reference : _references)
		{
			reference->uninit();
			delete reference;
		}
		for (ui::widget_id_t row : _rows)
			_ui->deallocate_widget(row);
		_inputs.resize(0);
		_references.resize(0);
		_rows.resize(0);
	}
}
