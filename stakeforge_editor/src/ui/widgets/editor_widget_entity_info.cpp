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
#include "commands/editor_commands_reflection.hpp"
#include "editor_app.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/reflection/reflection_registry.hpp>
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

		component_transform_t& get_transform_component(world_t& world, entity_id_t entity)
		{
			world_component_table_t* table = world.find_component_table(component_transform_t::TYPE_ID);
			SFG_ASSERT(table != nullptr);
			return ecs_helpers_t::table_get_as<component_transform_t>(table->table, entity);
		}

		void issue_transform_edit(world_handle_t world, entity_id_t entity, sid_t field_id, const component_transform_t& old_transform, const component_transform_t& new_transform)
		{
			ostream_t old_value;
			ostream_t new_value;
			if (!reflection_registry_t::get().serialize_field_to_stream(component_transform_t::TYPE_ID, field_id, &old_transform, old_value))
				return;
			if (!reflection_registry_t::get().serialize_field_to_stream(component_transform_t::TYPE_ID, field_id, &new_transform, new_value))
				return;

			editor_reflected_field_edit_desc_t desc = {};
			desc.target.world						= world;
			desc.target.entity						= entity;
			desc.target.type_id						= component_transform_t::TYPE_ID;
			desc.target.kind						= editor_reflected_edit_target_kind_e::world_component;
			desc.type_id							= component_transform_t::TYPE_ID;
			desc.field_id							= field_id;
			editor_commands_reflection_t::edit_field(desc, old_value, new_value);
		}
	}

	void editor_widget_entity_info_t::init(ui::ui_context& ui, ui::widget_id_t parent, world_handle_t world)
	{
		_ui							= &ui;
		_world						= &editor_app_t::get().get_runtime().get_world(world);
		_world_handle				= world;
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

		const editor_property_row_t	   name_row	   = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Name");
		editor_widget_text_id_config_t name_config = {};
		name_config.world						   = world;
		name_config.selected					   = on_name_selected;
		name_config.submitted					   = on_name_submitted;
		name_config.user_data					   = this;
		_name_input.init(ui, name_row.right, name_config);
		fit_control(ui, _name_input.get_root());

		const editor_property_row_t position_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Position");
		editor_vec3_field_config_t	position_config = {};
		position_config.on_changed					= on_position_changed;
		position_config.on_submitted				= on_position_submitted;
		position_config.user_data					= this;
		_position_field.init(ui, position_row.right, position_config);
		fit_control(ui, _position_field.get_root());

		const editor_property_row_t rotation_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Rotation");
		editor_vec3_field_config_t	rotation_config = {};
		rotation_config.on_changed					= on_rotation_changed;
		rotation_config.on_submitted				= on_rotation_submitted;
		rotation_config.user_data					= this;
		_rotation_field.init(ui, rotation_row.right, rotation_config);
		fit_control(ui, _rotation_field.get_root());

		const editor_property_row_t scale_row	 = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Scale");
		editor_vec3_field_config_t	scale_config = {};
		scale_config.value						 = vec3f_t::one;
		scale_config.on_changed					 = on_scale_changed;
		scale_config.on_submitted				 = on_scale_submitted;
		scale_config.user_data					 = this;
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

		_ui			   = nullptr;
		_world		   = nullptr;
		_root		   = NULL_WIDGET;
		_world_handle  = {};
		_command_rot   = {};
		_command_pos   = vec3f_t::zero;
		_command_scale = vec3f_t::one;
		_entity		   = NULL_ENTITY_ID;
		_refreshing	   = false;
	}

	void editor_widget_entity_info_t::set_entity(world_t& world, entity_id_t entity)
	{
		_world		= &world;
		_entity		= entity;
		_refreshing = true;

		_command_pos   = world.get_entity_pos_local(entity);
		_command_rot   = world.get_entity_rot_local(entity);
		_command_scale = world.get_entity_scale_local(entity);

		_name_input.refresh_text();
		_position_field.set_value(_command_pos);
		_rotation_field.set_value(quat_t::to_euler(_command_rot));
		_scale_field.set_value(_command_scale);

		_refreshing = false;
	}

	u32 editor_widget_entity_info_t::on_name_selected(void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._entity == NULL_ENTITY_ID)
			return ECS_INVALID_INDEX;

		const world_component_table_t* table = widget._world->find_component_table(component_name_t::TYPE_ID);
		SFG_ASSERT(table != nullptr);
		const component_name_t& name = ecs_helpers_t::table_get_as_const<component_name_t>(table->table, widget._entity);
		return name.text_index;
	}

	void editor_widget_entity_info_t::on_name_submitted(const char* value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		const char* old_text = widget._world->get_entity_name(widget._entity);

		editor_reflected_field_edit_desc_t desc = {};
		desc.target.world						= widget._world_handle;
		desc.target.entity						= widget._entity;
		desc.target.type_id						= component_name_t::TYPE_ID;
		desc.target.kind						= editor_reflected_edit_target_kind_e::world_component;
		desc.type_id							= component_name_t::TYPE_ID;
		desc.field_id							= "text_index"_hs;
		editor_commands_reflection_t::edit_text_id_field(desc, widget._world_handle, old_text != nullptr ? old_text : "", value != nullptr ? value : "");

		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel != nullptr)
			static_cast<editor_panel_entities_t*>(panel)->refresh_entity_name(widget._entity);
	}

	void editor_widget_entity_info_t::on_position_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_pos_local(widget._entity, value);
	}

	void editor_widget_entity_info_t::on_position_submitted(const vec3f_t&, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		const component_transform_t& current_transform = get_transform_component(*widget._world, widget._entity);
		component_transform_t		 old_transform	   = current_transform;
		old_transform.pos							   = widget._command_pos;
		issue_transform_edit(widget._world_handle, widget._entity, "pos"_hs, old_transform, current_transform);
		widget._command_pos = current_transform.pos;
	}

	void editor_widget_entity_info_t::on_rotation_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_rot_local(widget._entity, quat_t::from_euler(value.x, value.y, value.z));
	}

	void editor_widget_entity_info_t::on_rotation_submitted(const vec3f_t&, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		const component_transform_t& current_transform = get_transform_component(*widget._world, widget._entity);
		component_transform_t		 old_transform	   = current_transform;
		old_transform.rot							   = widget._command_rot;
		issue_transform_edit(widget._world_handle, widget._entity, "rot"_hs, old_transform, current_transform);
		widget._command_rot = current_transform.rot;
	}

	void editor_widget_entity_info_t::on_scale_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_scale_local(widget._entity, value);
	}

	void editor_widget_entity_info_t::on_scale_submitted(const vec3f_t&, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		const component_transform_t& current_transform = get_transform_component(*widget._world, widget._entity);
		component_transform_t		 old_transform	   = current_transform;
		old_transform.scale							   = widget._command_scale;
		issue_transform_edit(widget._world_handle, widget._entity, "scale"_hs, old_transform, current_transform);
		widget._command_scale = current_transform.scale;
	}
}
