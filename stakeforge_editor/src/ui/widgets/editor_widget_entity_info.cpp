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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ui/widgets/editor_widget_entity_info.hpp"
#include "editor_app.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		void fit_control(ui::ui_context& ui, ui::widget_id_t id)
		{
			const editor_theme_t& theme = editor_theme_t::get();

			ui::layout_in_t& in = ui.get_tree().in(id);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
			in.anchor_y			= ui::anchor_e::center;
			in.pos_value.y		= 0.5f;
			in.size_value		= {1.0f, theme.item_height};
		}

	}

	void editor_widget_entity_info_t::init(ui::ui_context& ui, ui::widget_id_t parent, world_handle_t world)
	{
		_ui							= &ui;
		_world						= &editor_app_t::get().get_runtime().get_world(world);
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "entity_info");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		const editor_property_row_t name_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Name");
		u8*							name_field	= reinterpret_cast<u8*>(_name_fallback);
		editor_input_field_config_t name_config = {};
		name_config.placeholder					= "Name";
		name_config.callbacks.edit_submitted	= on_name_input_submitted;
		name_config.callbacks.user_data			= this;
		name_config.field						= {.type = editor_input_field_field_type_e::char_array, .fields = {.data = &name_field, .size = 1}, .field_size = sizeof(component_name_t::text)};
		_name_input.init(ui, name_row.right, name_config);
		fit_control(ui, _name_input.get_root());

		const editor_property_row_t guid_row = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "GUID");
		_guid_label							 = ui.allocate_widget();
		ui.set_widget_debug_name(_guid_label, "entity_info_guid_label");
		tree.attach(guid_row.right, _guid_label);

		ui::layout_in_t& guid_label_in = tree.in(_guid_label);
		guid_label_in.flags			   = ui::wf_visible;
		guid_label_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		guid_label_in.size_mode_y	   = ui::axis_mode_e::fixed;
		guid_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		guid_label_in.anchor_y		   = ui::anchor_e::center;
		guid_label_in.pos_value.y	   = 0.5f;
		guid_label_in.size_value	   = {1.0f, theme.item_height};

		ui.set_widget_text(_guid_label, "");
		ui.get_paint().set_text(_guid_label,
								ui.widget_text(_guid_label),
								ui.widget_text_len(_guid_label),
								{.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		const editor_property_row_t position_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Position");
		editor_vec3_field_config_t	position_config = {};
		_position_field.init(ui, position_row.right, position_config);
		fit_control(ui, _position_field.get_root());

		const editor_property_row_t rotation_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Rotation");
		editor_quat_field_config_t	rotation_config = {};
		rotation_config.callbacks.edited			= on_rotation_changed;
		rotation_config.callbacks.user_data			= this;
		_rotation_field.init(ui, rotation_row.right, rotation_config);
		fit_control(ui, _rotation_field.get_root());

		const editor_property_row_t scale_row	 = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Scale");
		editor_vec3_field_config_t	scale_config = {};
		_scale_field.init(ui, scale_row.right, scale_config);
		fit_control(ui, _scale_field.get_root());
	}

	void editor_widget_entity_info_t::uninit()
	{
		_scale_field.uninit();
		_rotation_field.uninit();
		_position_field.uninit();
		_name_input.uninit();
		_ui->deallocate_widget(_root);

		_ui						  = nullptr;
		_world					  = nullptr;
		_root					  = NULL_WIDGET;
		_guid_label				  = NULL_WIDGET;
		_entity					  = NULL_ENTITY_ID;
		_name_fallback[0]		  = '\0';
		_name_submitted_callback  = nullptr;
		_name_submitted_user_data = nullptr;
		_entities.resize(0);
	}

	void editor_widget_entity_info_t::set_entity(world_t& world, entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_entities(world, {.data = entities, .size = 1});
	}

	void editor_widget_entity_info_t::set_entities(world_t& world, span_t<const entity_id_t> entities)
	{
		_world	= &world;
		_entity = entities.data[0];
		_entities.assign(entities.data, entities.data + entities.size);
		refresh_controls();
	}

	void editor_widget_entity_info_t::set_name_submitted_callback(editor_widget_entity_info_name_submitted_fn callback, void* user_data)
	{
		_name_submitted_callback  = callback;
		_name_submitted_user_data = user_data;
	}

	void editor_widget_entity_info_t::refresh_controls()
	{
		if (_entities.size() > 1)
		{
			_ui->set_widget_text(_guid_label, "Mixed");
		}
		else
		{
			char text[32] = {};
			std::snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(_world->get_entity_guid(_entity)));
			_ui->set_widget_text(_guid_label, text);
		}
		const editor_theme_t& theme = editor_theme_t::get();
		_ui->get_paint().set_text(_guid_label,
								  _ui->widget_text(_guid_label),
								  _ui->widget_text_len(_guid_label),
								  {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		frame_vector_t<u8*>		 name_values;
		frame_vector_t<vec3f_t*> position_values;
		frame_vector_t<quat_t*>	 rotation_values;
		frame_vector_t<vec3f_t*> scale_values;
		name_values.reserve(_entities.size());
		position_values.reserve(_entities.size());
		rotation_values.reserve(_entities.size());
		scale_values.reserve(_entities.size());

		world_component_table_t* name_table = _world->find_component_table(type_id_t<component_name_t>::value);
		SFG_ASSERT(name_table != nullptr);
		world_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);
		SFG_ASSERT(transform_table != nullptr);

		for (entity_id_t entity : _entities)
		{
			component_name_t&	   name		 = ecs_helpers_t::table_get_as<component_name_t>(name_table->table, entity);
			component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(transform_table->table, entity);
			name_values.push_back(reinterpret_cast<u8*>(name.text));
			position_values.push_back(&transform.pos);
			rotation_values.push_back(&transform.rot);
			scale_values.push_back(&transform.scale);
		}

		_name_input.update_field_data({.type = editor_input_field_field_type_e::char_array, .fields = {.data = name_values.data(), .size = name_values.size()}, .field_size = sizeof(component_name_t::text)});
		_position_field.update_field_data({.fields = {.data = position_values.data(), .size = position_values.size()}});
		_rotation_field.update_field_data({.fields = {.data = rotation_values.data(), .size = rotation_values.size()}});
		_scale_field.update_field_data({.fields = {.data = scale_values.data(), .size = scale_values.size()}});
	}

	void editor_widget_entity_info_t::on_name_input_submitted(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->submit_names();
	}

	void editor_widget_entity_info_t::on_rotation_changed(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->apply_rotation_values();
	}

	void editor_widget_entity_info_t::apply_rotation_values()
	{
		if (_entities.empty())
			return;

		world_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);
		SFG_ASSERT(transform_table != nullptr);
		for (entity_id_t entity : _entities)
		{
			const component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(transform_table->table, entity);
			_world->set_entity_rot_local(entity, transform.rot);
		}
	}

	void editor_widget_entity_info_t::submit_names()
	{
		if (_name_submitted_callback == nullptr)
			return;

		for (entity_id_t entity : _entities)
			_name_submitted_callback(entity, _name_submitted_user_data);
	}

}
