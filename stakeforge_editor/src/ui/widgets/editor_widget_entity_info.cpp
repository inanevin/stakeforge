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
#include "commands/editor_commands_entity_info.hpp"
#include "commands/editor_commands_reflection.hpp"
#include "editor_app.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/data/ostream.hpp>
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

		component_transform_t& get_transform_component(world_t& world, entity_id_t entity)
		{
			world_component_table_t* table = world.find_component_table(type_id_t<component_transform_t>::value);
			SFG_ASSERT(table != nullptr);
			return ecs_helpers_t::table_get_as<component_transform_t>(table->table, entity);
		}

		bool is_same_text(world_t& world, span_t<const entity_id_t> entities)
		{
			if (entities.size <= 1)
				return true;

			const char* first_name = world.get_entity_name(entities.data[0]);
			first_name			   = first_name != nullptr ? first_name : "";
			for (size_t i = 1; i < entities.size; ++i)
			{
				const char* name = world.get_entity_name(entities.data[i]);
				name			 = name != nullptr ? name : "";
				if (std::strcmp(first_name, name) != 0)
					return false;
			}
			return true;
		}

		bool is_same_pos(world_t& world, span_t<const entity_id_t> entities)
		{
			if (entities.size <= 1)
				return true;

			const vec3f_t& first = world.get_entity_pos_local(entities.data[0]);
			for (size_t i = 1; i < entities.size; ++i)
			{
				const vec3f_t& value = world.get_entity_pos_local(entities.data[i]);
				if (value.x != first.x || value.y != first.y || value.z != first.z)
					return false;
			}
			return true;
		}

		bool is_same_rot(world_t& world, span_t<const entity_id_t> entities)
		{
			if (entities.size <= 1)
				return true;

			const quat_t& first = world.get_entity_rot_local(entities.data[0]);
			for (size_t i = 1; i < entities.size; ++i)
			{
				const quat_t& value = world.get_entity_rot_local(entities.data[i]);
				if (value.x != first.x || value.y != first.y || value.z != first.z || value.w != first.w)
					return false;
			}
			return true;
		}

		bool is_same_scale(world_t& world, span_t<const entity_id_t> entities)
		{
			if (entities.size <= 1)
				return true;

			const vec3f_t& first = world.get_entity_scale_local(entities.data[0]);
			for (size_t i = 1; i < entities.size; ++i)
			{
				const vec3f_t& value = world.get_entity_scale_local(entities.data[i]);
				if (value.x != first.x || value.y != first.y || value.z != first.z)
					return false;
			}
			return true;
		}

		void issue_transform_edit(world_handle_t world, span_t<const entity_id_t> entities, sid_t field_id, const component_transform_t& old_transform, const component_transform_t& new_transform)
		{
			ostream_t old_value;
			ostream_t new_value;
			if (field_id == "pos"_hs)
			{
				old_value << old_transform.pos;
				new_value << new_transform.pos;
			}
			else if (field_id == "rot"_hs)
			{
				old_value << old_transform.rot;
				new_value << new_transform.rot;
			}
			else
			{
				SFG_ASSERT(field_id == "scale"_hs);
				old_value << old_transform.scale;
				new_value << new_transform.scale;
			}

			//editor_reflected_field_edit_desc_t desc = {};
			//desc.target.world						= world;
			//desc.target.entities					= entities.data;
			//desc.target.entity						= entities.data[0];
			//desc.target.type_id						= type_id_t<component_transform_t>::value;
			//desc.target.entity_count				= static_cast<u32>(entities.size);
			//desc.target.kind						= entities.size > 1 ? editor_reflected_edit_target_kind_e::world_components : editor_reflected_edit_target_kind_e::world_component;
			//desc.type_id							= type_id_t<component_transform_t>::value;
			//desc.field_id							= field_id;
			//editor_commands_reflection_t::edit_field(desc, old_value, new_value);
		}
	}

	void editor_widget_entity_info_t::init(ui::ui_context& ui, ui::widget_id_t parent, world_handle_t world)
	{
		_ui							= &ui;
		_world						= &editor_app_t::get().get_runtime().get_world(world);
		_world_handle				= world;
		_command_listener			= editor_app_t::get().get_command_system().add_listener(on_command_system_changed, this);
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
		//editor_widget_text_id_config_t name_config = {};
		//name_config.world						   = world;
		//name_config.selected					   = on_name_selected;
		//name_config.submitted					   = on_name_submitted;
		//name_config.user_data					   = this;
		//_name_input.init(ui, name_row.right, name_config);
		// fit_control(ui, _name_input.get_root());

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
		//_name_input.uninit();
		editor_app_t::get().get_command_system().remove_listener(_command_listener);
		_ui->deallocate_widget(_root);

		_ui				  = nullptr;
		_world			  = nullptr;
		_root			  = NULL_WIDGET;
		_guid_label		  = NULL_WIDGET;
		_world_handle	  = {};
		_command_listener = {};
		_command_rot	  = {};
		_command_pos	  = vec3f_t::zero;
		_command_scale	  = vec3f_t::one;
		_entity			  = NULL_ENTITY_ID;
		_entities.resize(0);
		_refreshing = false;
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

	void editor_widget_entity_info_t::refresh_controls()
	{
		_refreshing = true;

		_command_pos   = _world->get_entity_pos_local(_entity);
		_command_rot   = _world->get_entity_rot_local(_entity);
		_command_scale = _world->get_entity_scale_local(_entity);

	//	_name_input.refresh_text();
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
		_position_field.set_value(_command_pos);
		_rotation_field.set_value(quat_t::to_euler(_command_rot));
		_scale_field.set_value(_command_scale);
		//_name_input.set_mixed(!is_same_text(*_world, {.data = _entities.data(), .size = _entities.size()}));
		_position_field.set_mixed(!is_same_pos(*_world, {.data = _entities.data(), .size = _entities.size()}));
		_rotation_field.set_mixed(!is_same_rot(*_world, {.data = _entities.data(), .size = _entities.size()}));
		_scale_field.set_mixed(!is_same_scale(*_world, {.data = _entities.data(), .size = _entities.size()}));

		_refreshing = false;
	}

	u32 editor_widget_entity_info_t::on_name_selected(void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._entity == NULL_ENTITY_ID)
			return ECS_INVALID_INDEX;

		const world_component_table_t* table = widget._world->find_component_table(type_id_t<component_name_t>::value);
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

		//editor_reflected_field_edit_desc_t desc = {};
		//desc.target.world						= widget._world_handle;
		//desc.target.entities					= widget._entities.data();
		//desc.target.entity						= widget._entity;
		//desc.target.type_id						= type_id_t<component_name_t>::value;
		//desc.target.entity_count				= static_cast<u32>(widget._entities.size());
		//desc.target.kind						= widget._entities.size() > 1 ? editor_reflected_edit_target_kind_e::world_components : editor_reflected_edit_target_kind_e::world_component;
		//desc.type_id							= type_id_t<component_name_t>::value;
		//desc.field_id							= "text_index"_hs;
		//editor_commands_reflection_t::edit_text_id_field(desc, widget._world_handle, old_text != nullptr ? old_text : "", value != nullptr ? value : "");
		//
		//editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		//if (panel != nullptr)
		//{
		//	editor_panel_entities_t* entities_panel = static_cast<editor_panel_entities_t*>(panel);
		//	for (entity_id_t entity : widget._entities)
		//		entities_panel->refresh_entity_name(entity);
		//}
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
		issue_transform_edit(widget._world_handle, {.data = widget._entities.data(), .size = widget._entities.size()}, "pos"_hs, old_transform, current_transform);
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
		issue_transform_edit(widget._world_handle, {.data = widget._entities.data(), .size = widget._entities.size()}, "rot"_hs, old_transform, current_transform);
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
		issue_transform_edit(widget._world_handle, {.data = widget._entities.data(), .size = widget._entities.size()}, "scale"_hs, old_transform, current_transform);
		widget._command_scale = current_transform.scale;
	}

	void editor_widget_entity_info_t::on_command_system_changed(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		//editor_widget_entity_info_t&						 widget			   = *static_cast<editor_widget_entity_info_t*>(user_data);
		//const editor_command_reflected_field_edit_payload_t* reflected_payload = editor_commands_reflection_t::get_payload(system, command);
		//const bool											 reflected_match   = reflected_payload != nullptr && widget.matches_reflected_command(system, command);
		//const bool											 entity_info_match = widget.matches_entity_info_command(system, command);
		//if (!reflected_match && !entity_info_match)
		//	return;
		//
		//widget.refresh_controls();
		//if (entity_info_match || reflected_payload->target.type_id == type_id_t<component_name_t>::value)
		//	widget.refresh_entity_panel_names();
	}

	bool editor_widget_entity_info_t::matches_reflected_command(editor_command_system_t& system, const editor_command_t& command) const
	{
		return false;
		//const editor_command_reflected_field_edit_payload_t* payload = editor_commands_reflection_t::get_payload(system, command);
		//if (payload == nullptr)
		//	return false;
		//if (payload->target.world != _world_handle)
		//	return false;
		//if (payload->target.type_id != type_id_t<component_name_t>::value && payload->target.type_id != type_id_t<component_transform_t>::value)
		//	return false;
		//
		//switch (payload->target.kind)
		//{
		//case editor_reflected_edit_target_kind_e::world_component:
		//	for (entity_id_t entity : _entities)
		//	{
		//		if (entity == payload->target.entity)
		//			return true;
		//	}
		//	return false;
		//case editor_reflected_edit_target_kind_e::world_components: {
		//	SFG_ASSERT(payload->entities);
		//	const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload->entities);
		//	for (u32 i = 0; i < payload->entity_count; ++i)
		//	{
		//		for (entity_id_t selected_entity : _entities)
		//		{
		//			if (selected_entity == entities[i])
		//				return true;
		//		}
		//	}
		//	return false;
		//}
		//default:
		//	return false;
		//}
	}

	bool editor_widget_entity_info_t::matches_entity_info_command(editor_command_system_t& system, const editor_command_t& command) const
	{
		if (command.type != editor_command_type_e::entity_info_paste)
			return false;

		const editor_command_paste_entity_info_payload_t& payload = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
		if (payload.world != _world_handle)
			return false;

		SFG_ASSERT(payload.entities);
		const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload.entities);
		for (u32 i = 0; i < payload.count; ++i)
		{
			for (entity_id_t selected_entity : _entities)
			{
				if (selected_entity == entities[i])
					return true;
			}
		}
		return false;
	}

	void editor_widget_entity_info_t::refresh_entity_panel_names() const
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel != nullptr)
		{
			editor_panel_entities_t* entities_panel = static_cast<editor_panel_entities_t*>(panel);
			for (entity_id_t entity : _entities)
				entities_panel->refresh_entity_name(entity);
		}
	}
}
