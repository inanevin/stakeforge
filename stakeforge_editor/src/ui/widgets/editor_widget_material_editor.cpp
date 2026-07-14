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

#include "ui/widgets/editor_widget_material_editor.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_commands_material.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_checkbox.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void build_world_pass_items(frame_vector_t<editor_dropdown_item_t>& items)
		{
			const reflected_type_t* enum_type		= reflection_registry_t::get().find_type(type_id_t<world_pass_flags_e>::value);
			const u32				enum_item_count = enum_type->fields.end - enum_type->fields.start;
			items.reserve(enum_item_count);

			for (u32 i = 0; i < enum_item_count; ++i)
			{
				const reflected_field_t* enum_field = reflection_registry_t::get().get_field(enum_type->fields.start + i);
				const u32				 value		= i == 0 ? 0 : 1u << (i - 1);
				items.push_back({
					.text  = enum_field->display_name != nullptr ? enum_field->display_name : enum_field->name,
					.value = static_cast<u16>(value),
				});
			}
		}
	}

	void editor_widget_material_editor_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "material_editor");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
	}

	void editor_widget_material_editor_t::uninit()
	{
		_ui->cancel_mutations(this);
		clear_display();
		_ui->deallocate_widget(_root);

		_pending_material_ids.resize(0);
		_materials.resize(0);
		_material_ids.resize(0);
		_edit_previous_materials.resize(0);
		_edit_material_ids.resize(0);
		_shader_edit_previous_materials.resize(0);
		_shader_edit_material_ids.resize(0);
		_pass_flags.resize(0);
		_shader_definition		   = {};
		_ui						   = nullptr;
		_root					   = NULL_WIDGET;
		_has_shared_shader		   = false;
		_edit_active			   = false;
		_shader_edit_active		   = false;
		_refresh_materials_pending = false;
	}

	void editor_widget_material_editor_t::set_materials(span_t<const sid_t> materials)
	{
		if (!can_mutate_ui_topology())
		{
			request_materials_refresh(materials);
			return;
		}

		clear_material_edit();
		clear_shader_edit();
		clear_display();
		_materials.resize(0);
		_material_ids.resize(0);
		_pass_flags.resize(0);
		_shader_definition = {};
		_has_shared_shader = false;
		_materials.reserve(materials.size);
		_material_ids.reserve(materials.size);

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (size_t i = 0; i < materials.size; ++i)
		{
			const sid_t			  material_id = materials.data[i];
			const editor_asset_t* asset		  = assets.find_asset(material_id);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::material)
				continue;

			material_def_t		 material		 = {};
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			embedded_source.get_to(material);

			_material_ids.push_back(material_id);
			_materials.push_back(material);
		}

		refresh_display();
	}

	void editor_widget_material_editor_t::refresh_display()
	{
		if (_materials.empty())
			return;

		_has_shared_shader = load_shared_shader_definition();
		if (_has_shared_shader)
			normalize_materials_to_shader_definition();

		refresh_display_common();
		if (_has_shared_shader)
			refresh_display_data();
	}

	void editor_widget_material_editor_t::refresh_display_common()
	{
		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_material_edit_begin;
		callbacks.edited					= on_material_edited;
		callbacks.edit_submitted			= on_material_edit_submitted;
		callbacks.user_data					= this;

		editor_widget_callbacks_t shader_callbacks = {};
		shader_callbacks.edit_begin				   = on_shader_edit_begin;
		shader_callbacks.edited					   = on_shader_edited;
		shader_callbacks.edit_submitted			   = on_shader_edit_submitted;
		shader_callbacks.user_data				   = this;

		_labels.push_back(make_section_label("Material"));

		vector_t<u64*> shader_fields;
		shader_fields.reserve(_materials.size());
		for (material_def_t& material : _materials)
			shader_fields.push_back(reinterpret_cast<u64*>(&material.shader));

		const editor_property_row_t shader_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, "Shader");
		editor_widget_reference_t*	shader_ref = new editor_widget_reference_t();
		shader_ref->init(*_ui,
						 shader_row.right,
						 {
							 .callbacks	 = shader_callbacks,
							 .fields	 = {.data = shader_fields.data(), .size = shader_fields.size()},
							 .asset_type = editor_asset_type_e::shader,
							 .type		 = editor_widget_reference_type_e::asset,
						 });
		fit_control(shader_ref->get_root());
		_references.push_back(shader_ref);
		append_property_row(shader_row.row);

		_pass_flags.reserve(_materials.size());
		for (const material_def_t& material : _materials)
			_pass_flags.push_back(material.pass_flags.value());

		vector_t<u8*> pass_fields;
		pass_fields.reserve(_pass_flags.size());
		for (u32& pass_flags : _pass_flags)
			pass_fields.push_back(reinterpret_cast<u8*>(&pass_flags));

		frame_vector_t<editor_dropdown_item_t> pass_items;
		build_world_pass_items(pass_items);

		const editor_property_row_t pass_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, "Pass Mask");
		editor_dropdown_t*			pass	 = new editor_dropdown_t();
		pass->init(*_ui,
				   pass_row.right,
				   {
					   .items	   = pass_items.data(),
					   .field	   = {.fields = {.data = pass_fields.data(), .size = pass_fields.size()}, .field_size = sizeof(u32)},
					   .callbacks  = callbacks,
					   .item_count = static_cast<u16>(pass_items.size()),
					   .width	   = editor_dropdown_width_e::parent_relative,
					   .pos_y	   = editor_dropdown_pos_y_e::center,
					   .is_bitmask = true,
				   });
		fit_control(pass->get_root());
		_dropdowns.push_back(pass);
		append_property_row(pass_row.row);

		vector_t<u8*> double_sided_fields;
		double_sided_fields.reserve(_materials.size());
		for (material_def_t& material : _materials)
			double_sided_fields.push_back(reinterpret_cast<u8*>(&material.double_sided));

		const editor_property_row_t double_sided_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, "Double Sided");
		editor_checkbox_t*			double_sided	 = new editor_checkbox_t();
		double_sided->init(*_ui, double_sided_row.right, {.field = {.fields = {.data = double_sided_fields.data(), .size = double_sided_fields.size()}}, .callbacks = callbacks});
		_ui->get_tree().in(double_sided->get_root()).pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		_ui->get_tree().in(double_sided->get_root()).anchor_y	 = ui::anchor_e::center;
		_ui->get_tree().in(double_sided->get_root()).pos_value.y = 0.5f;
		_checkboxes.push_back(double_sided);
		append_property_row(double_sided_row.row);

		vector_t<u8*> alpha_cutoff_fields;
		alpha_cutoff_fields.reserve(_materials.size());
		for (material_def_t& material : _materials)
			alpha_cutoff_fields.push_back(reinterpret_cast<u8*>(&material.use_alpha_cutoff));

		const editor_property_row_t alpha_cutoff_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, "Alpha Cutoff");
		editor_checkbox_t*			alpha_cutoff	 = new editor_checkbox_t();
		alpha_cutoff->init(*_ui, alpha_cutoff_row.right, {.field = {.fields = {.data = alpha_cutoff_fields.data(), .size = alpha_cutoff_fields.size()}}, .callbacks = callbacks});
		_ui->get_tree().in(alpha_cutoff->get_root()).pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		_ui->get_tree().in(alpha_cutoff->get_root()).anchor_y	 = ui::anchor_e::center;
		_ui->get_tree().in(alpha_cutoff->get_root()).pos_value.y = 0.5f;
		_checkboxes.push_back(alpha_cutoff);
		append_property_row(alpha_cutoff_row.row);
	}

	void editor_widget_material_editor_t::refresh_display_data()
	{
		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_material_edit_begin;
		callbacks.edited					= on_material_edited;
		callbacks.edit_submitted			= on_material_edit_submitted;
		callbacks.user_data					= this;

		if (!_shader_definition.textures.empty())
			_labels.push_back(make_section_label("Textures"));

		for (size_t texture_index = 0; texture_index < _shader_definition.textures.size(); ++texture_index)
		{
			vector_t<u64*> fields;
			fields.reserve(_materials.size());
			for (material_def_t& material : _materials)
				fields.push_back(reinterpret_cast<u64*>(&material.textures[texture_index].texture));

			const char*					label	= _shader_definition.textures[texture_index].texture_name;
			const editor_property_row_t row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label != nullptr ? label : "Texture");
			editor_widget_reference_t*	control = new editor_widget_reference_t();
			control->init(*_ui,
						  row.right,
						  {
							  .callbacks  = callbacks,
							  .fields	  = {.data = fields.data(), .size = fields.size()},
							  .asset_type = editor_asset_type_e::texture,
							  .type		  = editor_widget_reference_type_e::asset,
						  });
			fit_control(control->get_root());
			_references.push_back(control);
			append_property_row(row.row);
		}

		if (!_shader_definition.samplers.empty())
			_labels.push_back(make_section_label("Samplers"));

		for (size_t sampler_index = 0; sampler_index < _shader_definition.samplers.size(); ++sampler_index)
		{
			vector_t<u64*> fields;
			fields.reserve(_materials.size());
			for (material_def_t& material : _materials)
				fields.push_back(reinterpret_cast<u64*>(&material.samplers[sampler_index].sampler));

			const char*					label	= _shader_definition.samplers[sampler_index].sampler_name;
			const editor_property_row_t row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label != nullptr ? label : "Sampler");
			editor_widget_reference_t*	control = new editor_widget_reference_t();
			control->init(*_ui,
						  row.right,
						  {
							  .callbacks  = callbacks,
							  .fields	  = {.data = fields.data(), .size = fields.size()},
							  .asset_type = editor_asset_type_e::texture_sampler,
							  .type		  = editor_widget_reference_type_e::asset,
						  });
			fit_control(control->get_root());
			_references.push_back(control);
			append_property_row(row.row);
		}

		if (!_shader_definition.parameters.empty())
			_labels.push_back(make_section_label("Material Parameters"));

		for (size_t parameter_index = 0; parameter_index < _shader_definition.parameters.size(); ++parameter_index)
		{
			const shader_param_definition_t& definition = _shader_definition.parameters[parameter_index];
			const char*						 label		= definition.param_name != nullptr ? definition.param_name : "Parameter";
			const editor_property_row_t		 row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label);

			switch (definition.type)
			{
			case shader_param_type_e::f32: {
				vector_t<u8*> fields;
				fields.reserve(_materials.size());
				for (material_def_t& material : _materials)
					fields.push_back(reinterpret_cast<u8*>(&material.parameters[parameter_index].value[0]));

				editor_input_field_t* control = new editor_input_field_t();
				control->init(*_ui,
							  row.right,
							  {
								  .field =
									  {
										  .fields	  = {.data = fields.data(), .size = fields.size()},
										  .field_size = sizeof(f32),
										  .type		  = editor_input_field_field_type_e::pod_number,
										  .is_slider  = true,
									  },
								  .callbacks   = callbacks,
								  .placeholder = label,
								  .increment   = 0.01f,
								  .min_value   = definition.min_value[0],
								  .max_value   = definition.max_value[0],
							  });
				fit_control(control->get_root());
				_inputs.push_back(control);
				break;
			}
			case shader_param_type_e::vec2: {
				vector_t<vec2f_t*> fields;
				fields.reserve(_materials.size());
				for (material_def_t& material : _materials)
					fields.push_back(reinterpret_cast<vec2f_t*>(material.parameters[parameter_index].value));

				editor_vec2_field_t* control = new editor_vec2_field_t();
				control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}}, .callbacks = callbacks});
				fit_control(control->get_root());
				_vec2_fields.push_back(control);
				break;
			}
			case shader_param_type_e::vec4: {
				if (definition.hint == shader_param_hint_e::color)
				{
					vector_t<color_t*> fields;
					fields.reserve(_materials.size());
					for (material_def_t& material : _materials)
						fields.push_back(reinterpret_cast<color_t*>(material.parameters[parameter_index].value));

					editor_color_field_t* control = new editor_color_field_t();
					control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}}, .callbacks = callbacks});
					fit_control(control->get_root());
					_color_fields.push_back(control);
					break;
				}

				vector_t<vec4f_t*> fields;
				fields.reserve(_materials.size());
				for (material_def_t& material : _materials)
					fields.push_back(reinterpret_cast<vec4f_t*>(material.parameters[parameter_index].value));

				editor_vec4_field_t* control = new editor_vec4_field_t();
				control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}}, .callbacks = callbacks});
				fit_control(control->get_root());
				_vec4_fields.push_back(control);
				break;
			}
			default:
				break;
			}

			append_property_row(row.row);
		}
	}

	void editor_widget_material_editor_t::clear_display()
	{
		for (editor_widget_reference_t* reference : _references)
		{
			reference->uninit();
			delete reference;
		}
		for (editor_dropdown_t* dropdown : _dropdowns)
		{
			dropdown->uninit();
			delete dropdown;
		}
		for (editor_checkbox_t* checkbox : _checkboxes)
		{
			checkbox->uninit();
			delete checkbox;
		}
		for (editor_color_field_t* color : _color_fields)
		{
			color->uninit();
			delete color;
		}
		for (editor_input_field_t* input : _inputs)
		{
			input->uninit();
			delete input;
		}
		for (editor_vec2_field_t* vec : _vec2_fields)
		{
			vec->uninit();
			delete vec;
		}
		for (editor_vec4_field_t* vec : _vec4_fields)
		{
			vec->uninit();
			delete vec;
		}
		for (ui::widget_id_t label : _labels)
			_ui->deallocate_widget(label);
		for (ui::widget_id_t row : _rows)
			_ui->deallocate_widget(row);
		for (ui::widget_id_t divider : _dividers)
			_ui->deallocate_widget(divider);

		_references.resize(0);
		_dropdowns.resize(0);
		_checkboxes.resize(0);
		_color_fields.resize(0);
		_inputs.resize(0);
		_vec2_fields.resize(0);
		_vec4_fields.resize(0);
		_rows.resize(0);
		_dividers.resize(0);
		_labels.resize(0);
	}

	void editor_widget_material_editor_t::fit_control(ui::widget_id_t widget)
	{
		ui::layout_in_t& in = _ui->get_tree().in(widget);
		in.size_mode_x		= ui::axis_mode_e::fill;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.anchor_y			= ui::anchor_e::center;
		in.pos_value.y		= 0.5f;
		in.size_value		= {1.0f, editor_theme_t::get().item_height};
	}

	void editor_widget_material_editor_t::append_property_row(ui::widget_id_t row)
	{
		_rows.push_back(row);
		_dividers.push_back(editor_dividers_t::add_divider_hor(*_ui, _root, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none));
	}

	ui::widget_id_t editor_widget_material_editor_t::make_section_label(const char* text)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t label = _ui->allocate_widget();
		_ui->set_widget_debug_name(label, "material_editor_section_label");
		_ui->get_tree().attach(_root, label);

		ui::layout_in_t& label_in = _ui->get_tree().in(label);
		label_in.flags			  = ui::wf_visible;
		label_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.item_area_height};
		label_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		label_in.flow			  = ui::flow_e::row;

		ui::widget_id_t text_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(text_widget, "material_editor_section_text");
		_ui->get_tree().attach(label, text_widget);

		ui::layout_in_t& text_in = _ui->get_tree().in(text_widget);
		text_in.flags			 = ui::wf_visible;
		text_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		text_in.pos_value.y		 = 0.5f;
		text_in.anchor_y		 = ui::anchor_e::center;
		text_in.size_mode_x		 = ui::axis_mode_e::fill;
		text_in.size_mode_y		 = ui::axis_mode_e::fixed;
		text_in.size_value		 = {1.0f, theme.text_default_px_size};

		_ui->set_widget_text(text_widget, text);
		_ui->get_paint().set_text(text_widget,
								  _ui->widget_text(text_widget),
								  _ui->widget_text_len(text_widget),
								  {.font = theme.font_title_bold, .color = theme.color_accent1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		return label;
	}

	bool editor_widget_material_editor_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	bool editor_widget_material_editor_t::load_shared_shader_definition()
	{
		const resource_handle_t shader = _materials[0].shader;
		for (size_t i = 1; i < _materials.size(); ++i)
		{
			if (_materials[i].shader != shader)
				return false;
		}

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(shader);
		if (asset == nullptr || asset->asset_type != editor_asset_type_e::shader || asset->embedded_source.empty())
			return false;

		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
		embedded_source.get_to(_shader_definition);
		return !_shader_definition.textures.empty() || !_shader_definition.samplers.empty() || !_shader_definition.parameters.empty();
	}

	void editor_widget_material_editor_t::normalize_materials_to_shader_definition()
	{
		for (material_def_t& material : _materials)
		{
			material_def_t normalized	= material_def_from_shader_def(_shader_definition, material.shader);
			normalized.pass_flags		= material.pass_flags;
			normalized.double_sided		= material.double_sided;
			normalized.use_alpha_cutoff = material.use_alpha_cutoff;

			for (material_texture_value_t& texture : normalized.textures)
			{
				const auto it = std::find_if(material.textures.begin(), material.textures.end(), [&](const material_texture_value_t& value) { return value.name == texture.name; });
				if (it != material.textures.end())
					texture.texture = it->texture;
			}

			for (material_sampler_value_t& sampler : normalized.samplers)
			{
				const auto it = std::find_if(material.samplers.begin(), material.samplers.end(), [&](const material_sampler_value_t& value) { return value.name == sampler.name; });
				if (it != material.samplers.end())
					sampler.sampler = it->sampler;
			}

			for (material_param_value_t& parameter : normalized.parameters)
			{
				const auto it = std::find_if(material.parameters.begin(), material.parameters.end(), [&](const material_param_value_t& value) { return value.name == parameter.name && value.type == parameter.type; });
				if (it != material.parameters.end())
				{
					parameter.hint = it->hint;
					for (u8 i = 0; i < 4; ++i)
						parameter.value[i] = it->value[i];
				}
			}

			material = normalized;
		}
	}

	void editor_widget_material_editor_t::sync_pass_flags()
	{
		for (size_t i = 0; i < _materials.size(); ++i)
			_materials[i].pass_flags = _pass_flags[i];
	}

	void editor_widget_material_editor_t::begin_material_edit()
	{
		clear_material_edit();
		_edit_previous_materials.assign(_materials.begin(), _materials.end());
		_edit_material_ids.assign(_material_ids.begin(), _material_ids.end());
		_edit_active = true;
	}

	void editor_widget_material_editor_t::submit_material_edit()
	{
		if (!_edit_active)
			return;

		if (!_pass_flags.empty())
			sync_pass_flags();
		editor_command_material_edit_t::edit({.data = _edit_material_ids.data(), .size = _edit_material_ids.size()}, {.data = _edit_previous_materials.data(), .size = _edit_previous_materials.size()}, {.data = _materials.data(), .size = _materials.size()});
		clear_material_edit();
		request_display_refresh();
	}

	void editor_widget_material_editor_t::clear_material_edit()
	{
		_edit_previous_materials.resize(0);
		_edit_material_ids.resize(0);
		_edit_active = false;
	}

	void editor_widget_material_editor_t::begin_shader_edit()
	{
		clear_shader_edit();
		_shader_edit_previous_materials.assign(_materials.begin(), _materials.end());
		_shader_edit_material_ids.assign(_material_ids.begin(), _material_ids.end());
		_shader_edit_active = true;
	}

	void editor_widget_material_editor_t::submit_shader_edit()
	{
		if (!_shader_edit_active)
			return;

		vector_t<resource_handle_t> post_shaders;
		post_shaders.reserve(_materials.size());
		for (const material_def_t& material : _materials)
			post_shaders.push_back(material.shader);

		editor_command_shader_edit_t::edit(
			{.data = _shader_edit_material_ids.data(), .size = _shader_edit_material_ids.size()}, {.data = _shader_edit_previous_materials.data(), .size = _shader_edit_previous_materials.size()}, {.data = post_shaders.data(), .size = post_shaders.size()});
		clear_shader_edit();
		request_display_refresh();
	}

	void editor_widget_material_editor_t::clear_shader_edit()
	{
		_shader_edit_previous_materials.resize(0);
		_shader_edit_material_ids.resize(0);
		_shader_edit_active = false;
	}

	void editor_widget_material_editor_t::request_materials_refresh(span_t<const sid_t> materials)
	{
		_pending_material_ids.resize(0);
		_pending_material_ids.reserve(materials.size);
		for (size_t i = 0; i < materials.size; ++i)
			_pending_material_ids.push_back(materials.data[i]);
		_refresh_materials_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_widget_material_editor_t::request_display_refresh()
	{
		_pending_material_ids.assign(_material_ids.begin(), _material_ids.end());
		_refresh_materials_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_widget_material_editor_t::flush_pending_ui_mutations()
	{
		if (!_refresh_materials_pending)
			return;

		_refresh_materials_pending = false;
		vector_t<sid_t> materials;
		materials.assign(_pending_material_ids.begin(), _pending_material_ids.end());
		_pending_material_ids.resize(0);
		set_materials({.data = materials.data(), .size = materials.size()});
	}

	void editor_widget_material_editor_t::on_material_edit_begin()
	{
		begin_material_edit();
	}

	void editor_widget_material_editor_t::on_material_edited()
	{
		if (!_pass_flags.empty())
			sync_pass_flags();
	}

	void editor_widget_material_editor_t::on_material_edit_submitted()
	{
		submit_material_edit();
	}

	void editor_widget_material_editor_t::on_shader_edit_begin()
	{
		begin_shader_edit();
	}

	void editor_widget_material_editor_t::on_shader_edited()
	{
	}

	void editor_widget_material_editor_t::on_shader_edit_submitted()
	{
		submit_shader_edit();
	}

	void editor_widget_material_editor_t::on_material_edit_begin(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edit_begin();
	}

	void editor_widget_material_editor_t::on_material_edited(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edited();
	}

	void editor_widget_material_editor_t::on_material_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edit_submitted();
	}

	void editor_widget_material_editor_t::on_shader_edit_begin(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_shader_edit_begin();
	}

	void editor_widget_material_editor_t::on_shader_edited(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_shader_edited();
	}

	void editor_widget_material_editor_t::on_shader_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_shader_edit_submitted();
	}

	void editor_widget_material_editor_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->flush_pending_ui_mutations();
	}
}
