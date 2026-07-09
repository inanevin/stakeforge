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
#include "ui/widgets/editor_widget_inspector.hpp"
#include "world_edit/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "commands/editor_command_component_edit.hpp"
#include "commands/editor_commands_component.hpp"
#include "editor_app.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		enum inspector_component_action_menu_command_e : u16
		{
			inspector_component_action_menu_copy = 1,
			inspector_component_action_menu_paste,
			inspector_component_action_menu_reset,
			inspector_component_action_menu_remove,
		};

		enum inspector_entity_info_action_menu_command_e : u16
		{
			inspector_entity_info_action_menu_copy = 1,
			inspector_entity_info_action_menu_paste,
		};

		bool get_selected_entities_from_panel(editor_world_handle_t edit_world, frame_vector_t<entity_id_t>& entities, editor_world_handle_t& world)
		{
			editor_world_edit_context_t&	controller = editor_world_controller_t::get().get_editor_world(edit_world)->get_edit_context();
			const span_t<const entity_id_t> selected   = controller.get_selected_entities();
			if (selected.size == 0)
				return false;

			world = controller.get_world();
			entities.reserve(selected.size);
			for (size_t i = 0; i < selected.size; ++i)
				entities.push_back(selected.data[i]);
			return !world.is_null();
		}
	}
	void editor_widget_inspector_t::on_entity_info_settings_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_inspector_t*>(user_data)->open_entity_info_action_menu(pos);
	}

	void editor_widget_inspector_t::on_entity_info_action_menu_command(u16 command, void* user_data)
	{
		editor_widget_inspector_t& panel = *static_cast<editor_widget_inspector_t*>(user_data);
		switch (command)
		{
		case inspector_entity_info_action_menu_copy:
			panel.copy_entity_info();
			break;
		case inspector_entity_info_action_menu_paste: {
			frame_vector_t<entity_id_t> entities;
			editor_world_handle_t		world = {};
			if (get_selected_entities_from_panel(panel._edit_world, entities, world) && panel._copied_entity_info_valid)
				editor_commands_entity_info_t::paste(world, entities, panel._copied_entity_info);
			break;
		}
		default:
			break;
		}
	}

	void editor_widget_inspector_t::on_entity_info_name_submitted(entity_id_t entity, void*)
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel == nullptr)
			return;

		static_cast<editor_panel_entities_t*>(panel)->refresh_entity_name(entity);
	}

	void editor_widget_inspector_t::on_entity_info_edit_begin(void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->begin_entity_info_edit();
	}

	void editor_widget_inspector_t::on_entity_info_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->submit_entity_info_edit();
	}

	void editor_widget_inspector_t::on_entity_info_break_prefab(void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->break_prefabs();
	}

	void editor_widget_inspector_t::on_component_settings_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_inspector_t& panel = *static_cast<editor_widget_inspector_t*>(user_data);
		for (const component_display_t& display : panel._component_displays)
		{
			if (display.fold->get_settings_button() == id)
			{
				panel.open_component_action_menu(pos, display.type_id);
				return;
			}
		}
	}

	void editor_widget_inspector_t::on_component_action_menu_command(u16 command, void* user_data)
	{
		editor_widget_inspector_t& panel = *static_cast<editor_widget_inspector_t*>(user_data);
		switch (command)
		{
		case inspector_component_action_menu_copy:
			panel.copy_component(panel._action_menu_type_id);
			break;
		case inspector_component_action_menu_paste: {
			frame_vector_t<entity_id_t> entities;
			editor_world_handle_t		world = {};
			if (get_selected_entities_from_panel(panel._edit_world, entities, world) && panel.is_component_paste_enabled(panel._action_menu_type_id))
				editor_commands_component_t::paste(world, entities, panel._action_menu_type_id, panel._copied_component_stream.get_raw(), panel._copied_component_stream.get_size());
			break;
		}
		case inspector_component_action_menu_reset: {
			frame_vector_t<entity_id_t> entities;
			editor_world_handle_t		world = {};
			if (get_selected_entities_from_panel(panel._edit_world, entities, world))
				editor_commands_component_t::reset(world, entities, panel._action_menu_type_id);
			break;
		}
		case inspector_component_action_menu_remove: {
			frame_vector_t<entity_id_t> entities;
			editor_world_handle_t		world = {};
			if (get_selected_entities_from_panel(panel._edit_world, entities, world))
				editor_commands_component_t::remove(world, entities, panel._action_menu_type_id);
			break;
		}
		default:
			break;
		}
	}

	void editor_widget_inspector_t::on_add_component_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_inspector_t*>(user_data)->open_add_component_action_menu(pos);
	}

	void editor_widget_inspector_t::on_add_component_action_menu_command(u16 command, void* user_data)
	{
		editor_widget_inspector_t& panel = *static_cast<editor_widget_inspector_t*>(user_data);
		if (command == 0 || command > panel._add_component_types.size())
			return;

		frame_vector_t<entity_id_t> entities;
		editor_world_handle_t		world = {};
		if (get_selected_entities_from_panel(panel._edit_world, entities, world))
			editor_commands_component_t::add(world, entities, panel._add_component_types[command - 1]);
	}

	void editor_widget_inspector_t::on_component_edit_begin(void* user_data)
	{
		component_edit_callback_data_t& data = *static_cast<component_edit_callback_data_t*>(user_data);
		data.panel->begin_component_edit(data.component_type);
	}

	void editor_widget_inspector_t::on_component_edit_submitted(void* user_data)
	{
		component_edit_callback_data_t& data = *static_cast<component_edit_callback_data_t*>(user_data);
		data.panel->submit_component_edit(data.component_type);
	}

	void editor_widget_inspector_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_widget_inspector_t& panel = *static_cast<editor_widget_inspector_t*>(user_data);
		switch (command.type)
		{
		case editor_command_type_e::component_add: {
			const editor_command_add_component_payload_t& payload  = system.get_payload_as<editor_command_add_component_payload_t>(command);
			const entity_id_t*							  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_display();
			break;
		}
		case editor_command_type_e::component_remove: {
			const editor_command_remove_component_payload_t& payload  = system.get_payload_as<editor_command_remove_component_payload_t>(command);
			const entity_id_t*								 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_display();
			break;
		}
		case editor_command_type_e::component_reset: {
			const editor_command_reset_component_payload_t& payload	 = system.get_payload_as<editor_command_reset_component_payload_t>(command);
			const entity_id_t*								entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_component_reflection(payload.component_type);
			break;
		}
		case editor_command_type_e::component_paste: {
			const editor_command_paste_component_payload_t& payload	 = system.get_payload_as<editor_command_paste_component_payload_t>(command);
			const entity_id_t*								entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_component_reflection(payload.component_type);
			break;
		}
		case editor_command_type_e::component_edit: {
			const editor_command_component_edit_payload_t& payload	= system.get_payload_as<editor_command_component_edit_payload_t>(command);
			const entity_id_t*							   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_component_reflection(payload.component_type);
			break;
		}
		case editor_command_type_e::entity_info_paste: {
			const editor_command_paste_entity_info_payload_t& payload  = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			const entity_id_t*								  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_display();
			break;
		}
		case editor_command_type_e::entity_info_edit: {
			const editor_command_edit_entity_info_payload_t& payload  = system.get_payload_as<editor_command_edit_entity_info_payload_t>(command);
			const entity_id_t*								 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			if (payload.world == panel._edit_world && panel.is_displaying_any_entity({.data = entities, .size = payload.count}))
				panel.refresh_display();
			break;
		}
		default:
			break;
		}
	}

	void editor_widget_inspector_t::on_selection_changed(editor_world_edit_context_t&, void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->refresh_from_selection();
	}

	void editor_widget_inspector_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->flush_pending_ui_mutations();
	}

	void editor_widget_inspector_t::on_scroll_restore_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_widget_inspector_t*>(user_data)->apply_pending_scroll_restore();
	}
}
