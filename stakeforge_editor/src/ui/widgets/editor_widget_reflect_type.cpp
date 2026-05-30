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
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

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
			tree.draw_order(label) = tree.draw_order_const(parent) + 1;

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
	}

	void editor_widget_reflect_type_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reflect_type");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

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

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			const editor_property_row_t	  row	= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, field.display_name != nullptr ? field.display_name : field.name);
			_rows.push_back(row);

			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				editor_input_field_t*		input  = new editor_input_field_t();
				editor_input_field_config_t config = {};
				apply_reflected_number_config(config, field, false);
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
				input->init(*_ui, row.right, config);
				_input_fields.push_back(input);
				break;
			}
			case reflected_value_type_e::bool8: {
				editor_checkbox_t* checkbox = new editor_checkbox_t();
				checkbox->init(*_ui, row.right, {});
				_checkboxes.push_back(checkbox);
				break;
			}
			case reflected_value_type_e::vec2: {
				editor_vec2_field_t* vec = new editor_vec2_field_t();
				vec->init(*_ui, row.right, {});
				_vec2_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::vec3: {
				editor_vec3_field_t* vec = new editor_vec3_field_t();
				vec->init(*_ui, row.right, {});
				_vec3_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::vec4: {
				editor_vec4_field_t* vec = new editor_vec4_field_t();
				vec->init(*_ui, row.right, {});
				_vec4_fields.push_back(vec);
				break;
			}
			case reflected_value_type_e::color: {
				editor_color_field_t* color = new editor_color_field_t();
				color->init(*_ui, row.right, {});
				_color_fields.push_back(color);
				break;
			}
			case reflected_value_type_e::string: {
				editor_input_field_t*		input  = new editor_input_field_t();
				editor_input_field_config_t config = {};
				config.type						   = editor_input_field_type_e::text;
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
	}
}
