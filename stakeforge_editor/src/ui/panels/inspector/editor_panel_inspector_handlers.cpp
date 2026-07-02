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
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "commands/editor_commands_component.hpp"
#include "commands/editor_commands_entity_info.hpp"

#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_entity_info.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

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

		bool get_selected_entities_from_panel(frame_vector_t<entity_id_t>& entities, world_handle_t& world)
		{
			editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
			if (panel == nullptr)
				return false;

			editor_panel_entities_t*		entities_panel = static_cast<editor_panel_entities_t*>(panel);
			const span_t<const entity_id_t> selected	   = entities_panel->get_selected_entities();
			if (selected.size == 0)
				return false;

			world = entities_panel->get_world();
			entities.reserve(selected.size);
			for (size_t i = 0; i < selected.size; ++i)
				entities.push_back(selected.data[i]);
			return !world.is_null();
		}
	}
	void editor_panel_inspector_t::on_entity_info_settings_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_panel_inspector_t*>(user_data)->open_entity_info_action_menu(pos);
	}

	void editor_panel_inspector_t::on_entity_info_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		switch (command)
		{
		case inspector_entity_info_action_menu_copy:
			panel.copy_entity_info();
			break;
		case inspector_entity_info_action_menu_paste: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world) && panel._copied_entity_info_valid)
				editor_commands_entity_info_t::paste(world, entities, panel._copied_entity_info);
			break;
		}
		default:
			break;
		}
	}

	void editor_panel_inspector_t::on_entity_info_name_submitted(entity_id_t entity, void*)
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel == nullptr)
			return;

		static_cast<editor_panel_entities_t*>(panel)->refresh_entity_name(entity);
	}

	void editor_panel_inspector_t::on_component_settings_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		for (const component_display_t& display : panel._component_displays)
		{
			if (display.fold->get_settings_button() == id)
			{
				panel.open_component_action_menu(pos, display.type_id);
				return;
			}
		}
	}

	void editor_panel_inspector_t::on_component_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		switch (command)
		{
		case inspector_component_action_menu_copy:
			panel.copy_component(panel._action_menu_type_id);
			break;
		case inspector_component_action_menu_paste: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world) && panel.is_component_paste_enabled(panel._action_menu_type_id))
				editor_commands_component_t::paste(world, entities, panel._action_menu_type_id, panel._copied_component_stream.get_raw(), panel._copied_component_stream.get_size());
			break;
		}
		case inspector_component_action_menu_reset: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world))
				editor_commands_component_t::reset(world, entities, panel._action_menu_type_id);
			break;
		}
		case inspector_component_action_menu_remove: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world))
				editor_commands_component_t::remove(world, entities, panel._action_menu_type_id);
			break;
		}
		default:
			break;
		}
	}

	void editor_panel_inspector_t::on_add_component_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_panel_inspector_t*>(user_data)->open_add_component_action_menu(pos);
	}

	void editor_panel_inspector_t::on_add_component_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		if (command == 0 || command > panel._add_component_types.size())
			return;

		frame_vector_t<entity_id_t> entities;
		world_handle_t				world = {};
		if (get_selected_entities_from_panel(entities, world) && editor_commands_component_t::add(world, entities, panel._add_component_types[command - 1]))
			panel.refresh_display();
	}

	void editor_panel_inspector_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		if (command.type != editor_command_type_e::component_reset)
			return;

		editor_panel_inspector_t&						panel	= *static_cast<editor_panel_inspector_t*>(user_data);
		const editor_command_reset_component_payload_t& payload = system.get_payload_as<editor_command_reset_component_payload_t>(command);
		if (payload.world != panel._display_world_handle)
			return;

		panel.request_refresh_component_reflection(payload.component_type);
	}

	void editor_panel_inspector_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_inspector_t*>(user_data)->flush_pending_ui_mutations();
	}

	void editor_panel_inspector_t::on_scroll_restore_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_panel_inspector_t*>(user_data)->apply_pending_scroll_restore();
	}
}
