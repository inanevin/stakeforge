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
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_checkbox.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
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

		_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Material"));

		vector_t<u64*> shader_fields = {};
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

		const auto append_bool_property = [&](const char* label, bool material_def_t::* member) {
			vector_t<u8*> fields = {};
			fields.reserve(_materials.size());

			for (material_def_t& material : _materials)
				fields.push_back(reinterpret_cast<u8*>(&(material.*member)));

			const editor_property_row_t row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label);
			editor_checkbox_t*			control = new editor_checkbox_t();
			control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}}, .callbacks = callbacks});
			_ui->get_tree().in(control->get_root()).pos_mode_y	= ui::pos_mode_e::relative_in_parent;
			_ui->get_tree().in(control->get_root()).anchor_y	= ui::anchor_e::center;
			_ui->get_tree().in(control->get_root()).pos_value.y = 0.5f;
			_checkboxes.push_back(control);
			append_property_row(row.row);
		};

		append_bool_property("Write Shadows", &material_def_t::write_shadows);
		append_bool_property("Write Reflections", &material_def_t::write_reflections);
		append_bool_property("Double Sided", &material_def_t::double_sided);
		append_bool_property("Alpha Cutoff", &material_def_t::use_alpha_cutoff);

		const editor_asset_t* shader_asset		 = _materials.empty() ? nullptr : editor_asset_manager_t::get().find_asset(_materials.front().shader);
		const bool			  is_particle_shader = shader_asset != nullptr && shader_asset->asset_type == editor_asset_type_e::shader && shader_asset->sub_type == static_cast<u8>(shader_type_e::particle_shader);
		vector_t<u8*>		  fields			 = {};
		fields.reserve(_materials.size());

		for (material_def_t& material : _materials)
			fields.push_back(reinterpret_cast<u8*>(&material.blend_mode));

		const editor_property_row_t row		 = editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, "Blend Mode");
		editor_dropdown_t*			dropdown = new editor_dropdown_t();

		if (is_particle_shader)
		{
			const editor_dropdown_item_t items[] = {
				{.text = "Alpha", .value = static_cast<u64>(material_blend_mode_e::alpha)},
				{.text = "Premultiplied Alpha", .value = static_cast<u64>(material_blend_mode_e::premultiplied_alpha)},
				{.text = "Additive", .value = static_cast<u64>(material_blend_mode_e::additive)},
			};

			dropdown->init(*_ui,
						   row.right,
						   {
							   .items	   = items,
							   .field	   = {.fields = {.data = fields.data(), .size = fields.size()}, .field_size = sizeof(material_blend_mode_e)},
							   .callbacks  = callbacks,
							   .item_count = static_cast<u16>(std::size(items)),
							   .width	   = editor_dropdown_width_e::parent_relative,
							   .pos_y	   = editor_dropdown_pos_y_e::center,
						   });
		}
		else
		{
			const editor_dropdown_item_t items[] = {
				{.text = "Opaque", .value = static_cast<u64>(material_blend_mode_e::opaque)},
				{.text = "Alpha", .value = static_cast<u64>(material_blend_mode_e::alpha)},
			};

			dropdown->init(*_ui,
						   row.right,
						   {
							   .items	   = items,
							   .field	   = {.fields = {.data = fields.data(), .size = fields.size()}, .field_size = sizeof(material_blend_mode_e)},
							   .callbacks  = callbacks,
							   .item_count = static_cast<u16>(std::size(items)),
							   .width	   = editor_dropdown_width_e::parent_relative,
							   .pos_y	   = editor_dropdown_pos_y_e::center,
						   });
		}

		fit_control(dropdown->get_root());
		_dropdowns.push_back(dropdown);
		append_property_row(row.row);
	}

	void editor_widget_material_editor_t::refresh_display_data()
	{
		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_material_edit_begin;
		callbacks.edited					= on_material_edited;
		callbacks.edit_submitted			= on_material_edit_submitted;
		callbacks.user_data					= this;

		if (!_shader_definition.textures.empty())
			_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Textures"));

		for (size_t texture_index = 0; texture_index < _shader_definition.textures.size(); ++texture_index)
		{
			vector_t<u64*> fields = {};
			fields.reserve(_materials.size());
			for (material_def_t& material : _materials)
				fields.push_back(reinterpret_cast<u64*>(&material.textures[texture_index].texture));

			const char*			label	   = _shader_definition.textures[texture_index].texture_name;
			editor_asset_type_e asset_type = editor_asset_type_e::texture;

			switch (_shader_definition.textures[texture_index].type)
			{
			case shader_texture_type_e::texture_cube:
				asset_type = editor_asset_type_e::cubemap;
				break;
			case shader_texture_type_e::sprite:
				asset_type = editor_asset_type_e::sprite;
				break;
			default:
				break;
			}

			const editor_property_row_t row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label != nullptr ? label : "Texture");
			editor_widget_reference_t*	control = new editor_widget_reference_t();
			control->init(*_ui,
						  row.right,
						  {
							  .callbacks  = callbacks,
							  .fields	  = {.data = fields.data(), .size = fields.size()},
							  .asset_type = asset_type,
							  .type		  = editor_widget_reference_type_e::asset,
						  });
			fit_control(control->get_root());
			_references.push_back(control);
			append_property_row(row.row);
		}

		if (!_shader_definition.samplers.empty())
			_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Samplers"));

		for (size_t sampler_index = 0; sampler_index < _shader_definition.samplers.size(); ++sampler_index)
		{
			vector_t<u64*> fields = {};
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
			_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Material Parameters"));

		for (size_t parameter_index = 0; parameter_index < _shader_definition.parameters.size(); ++parameter_index)
		{
			const shader_param_definition_t& definition = _shader_definition.parameters[parameter_index];
			const char*						 label		= definition.param_name != nullptr ? definition.param_name : "Parameter";
			const editor_property_row_t		 row		= editor_misc_widgets_t::make_property_row_with_label(*_ui, _root, label);

			if (definition.hint == shader_param_hint_e::toggle)
			{
				vector_t<u8*> fields = {};
				fields.reserve(_materials.size());
				for (material_def_t& material : _materials)
					fields.push_back(reinterpret_cast<u8*>(&material.parameters[parameter_index].value_u32[0]));

				editor_checkbox_t* control = new editor_checkbox_t();
				control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}, .field_size = sizeof(u32)}, .callbacks = callbacks});
				_ui->get_tree().in(control->get_root()).pos_mode_y	= ui::pos_mode_e::relative_in_parent;
				_ui->get_tree().in(control->get_root()).anchor_y	= ui::anchor_e::center;
				_ui->get_tree().in(control->get_root()).pos_value.y = 0.5f;
				_checkboxes.push_back(control);
				append_property_row(row.row);
				continue;
			}

			switch (definition.type)
			{
			case shader_param_type_e::f32: {
				vector_t<u8*> fields = {};
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
			case shader_param_type_e::u32: {
				vector_t<u8*> fields = {};
				fields.reserve(_materials.size());
				for (material_def_t& material : _materials)
					fields.push_back(reinterpret_cast<u8*>(&material.parameters[parameter_index].value_u32[0]));

				editor_input_field_t* control = new editor_input_field_t();
				control->init(*_ui,
							  row.right,
							  {
								  .field =
									  {
										  .fields	  = {.data = fields.data(), .size = fields.size()},
										  .field_size = sizeof(u32),
										  .type		  = editor_input_field_field_type_e::pod_number,
										  .is_slider  = true,
									  },
								  .callbacks   = callbacks,
								  .placeholder = label,
								  .increment   = 1.0f,
								  .min_value   = static_cast<f32>(definition.min_value_u32[0]),
								  .max_value   = static_cast<f32>(definition.max_value_u32[0]),
								  .is_integer  = true,
							  });
				fit_control(control->get_root());
				_inputs.push_back(control);
				break;
			}
			case shader_param_type_e::vec2: {
				vector_t<vec2f_t*> fields = {};
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
					vector_t<color_t*> fields = {};
					fields.reserve(_materials.size());
					for (material_def_t& material : _materials)
						fields.push_back(reinterpret_cast<color_t*>(material.parameters[parameter_index].value));

					editor_color_field_t* control = new editor_color_field_t();
					control->init(*_ui, row.right, {.field = {.fields = {.data = fields.data(), .size = fields.size()}}, .callbacks = callbacks});
					fit_control(control->get_root());
					_color_fields.push_back(control);
					break;
				}

				vector_t<vec4f_t*> fields = {};
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
		for (editor_dropdown_t* dropdown : _dropdowns)
		{
			dropdown->uninit();
			delete dropdown;
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
		_checkboxes.resize(0);
		_color_fields.resize(0);
		_dropdowns.resize(0);
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
			material_def_t normalized	 = material_def_from_shader_def(_shader_definition, material.shader);
			normalized.blend_mode		 = material.blend_mode;
			normalized.write_shadows	 = material.write_shadows;
			normalized.write_reflections = material.write_reflections;
			normalized.double_sided		 = material.double_sided;
			normalized.use_alpha_cutoff	 = material.use_alpha_cutoff;

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
					{
						if (parameter.type == shader_param_type_e::u32)
							parameter.value_u32[i] = it->value_u32[i];
						else
							parameter.value[i] = it->value[i];
					}
				}
			}

			material = normalized;
		}
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

		editor_command_material_edit_t::edit({.data = _edit_material_ids.data(), .size = _edit_material_ids.size()}, {.data = _edit_previous_materials.data(), .size = _edit_previous_materials.size()}, {.data = _materials.data(), .size = _materials.size()});
		clear_material_edit();
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

		vector_t<resource_handle_t> post_shaders = {};
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
		vector_t<sid_t> materials  = {};
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
