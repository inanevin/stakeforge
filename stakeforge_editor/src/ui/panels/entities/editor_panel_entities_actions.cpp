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
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/editor_payload_controller.hpp"
#include "editor_selection_controller.hpp"
#include "editor_world_metadata.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "commands/editor_commands_component.hpp"
#include "commands/editor_commands_entity.hpp"
#include "commands/editor_commands_world_metadata.hpp"
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>

namespace sfg
{
	namespace
	{
		editor_action_menu_row_desc_t ENTITY_CREATE_ROWS[] = {
			{.text = "Empty Entity", .command = entity_action_menu_create_empty},
			{.text = "Folder", .command = entity_action_menu_create_folder},
		};

		editor_action_menu_row_desc_t ENTITY_CREATE_ENTITY_ROWS[] = {
			{.text = "Empty Entity", .command = entity_action_menu_create_empty},
			{.text = "Folder", .command = entity_action_menu_create_folder, .disabled = true},
		};

		editor_action_menu_row_desc_t ENTITY_EMPTY_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ENTITY_ROW_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ENTITY_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ENTITY_ROWS) / sizeof(ENTITY_CREATE_ENTITY_ROWS[0]))},
			{.text = "Duplicate Entity", .shortcut = "CTRL+D", .command = entity_action_menu_duplicate},
			{.text = "Delete Entity", .shortcut = "DEL", .command = entity_action_menu_delete},
		};

		editor_action_menu_row_desc_t FOLDER_ROW_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
			{.text = "Rename", .shortcut = "F2", .command = entity_action_menu_rename_folder},
			{.text = "Change Color", .command = entity_action_menu_change_folder_color},
			{.text = "Delete", .command = entity_action_menu_delete_folder},
		};
	}
	void editor_panel_entities_t::create_entity(entity_id_t parent, editor_world_folder_handle_t folder)
	{
		const world_handle_t main_world = editor_world_controller_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		const entity_id_t entity = editor_commands_entity_t::create(main_world, parent, folder);
		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
		{
			world_t& world = editor_world_controller_t::get().get_world(main_world);
			editor_world_metadata_t::get().set_entity_folded(world.get_entity_guid(parent), false);
		}
		if (!folder.is_null())
			editor_world_metadata_t::get().set_folder_folded(folder, false);
		editor_selection_controller_t::get().issue_entity_selection({.data = &entity, .size = 1}, entity);
		refresh_entities();
	}

	void editor_panel_entities_t::create_folder(editor_world_folder_handle_t parent)
	{
		const editor_world_folder_handle_t folder = editor_commands_world_metadata_t::create_folder("Folder", parent);
		if (!parent.is_null())
			editor_world_metadata_t::get().set_folder_folded(parent, false);
		_focused_folder = folder;
		refresh_entities();
	}

	void editor_panel_entities_t::delete_folder(editor_world_folder_handle_t folder)
	{
		if (!editor_commands_world_metadata_t::delete_folder(folder))
			return;

		_action_menu_folder = {};
		_focused_folder		= {};
		_edit_folder		= {};
		refresh_entities();
	}

	bool editor_panel_entities_t::assign_payload_entities_to_folder(const vector_t<editor_entity_payload_t>& entities, editor_world_folder_handle_t folder)
	{
		if (entities.empty())
			return false;

		const world_handle_t		  main_world = editor_world_controller_t::get().get_main_world();
		world_t&					  world		 = editor_world_controller_t::get().get_world(main_world);
		frame_vector_t<entity_id_t>	  moved_entities;
		frame_vector_t<entity_guid_t> moved_guids;
		moved_entities.reserve(entities.size());
		moved_guids.reserve(entities.size());
		for (const editor_entity_payload_t& payload_entity : entities)
		{
			if (!(payload_entity.world == main_world) || payload_entity.entity == NULL_ENTITY_ID || !world.is_alive(payload_entity.entity))
				return false;
			moved_entities.push_back(payload_entity.entity);
			moved_guids.push_back(world.get_entity_guid(payload_entity.entity));
		}

		if (!editor_commands_entity_t::reparent(main_world, moved_entities, NULL_ENTITY_ID))
			return false;

		if (!editor_commands_world_metadata_t::assign_entities_to_folder(folder, {.data = moved_guids.data(), .size = moved_guids.size()}))
			return false;

		editor_world_metadata_t::get().set_folder_folded(folder, false);
		refresh_entities();
		return true;
	}

	bool editor_panel_entities_t::deassign_payload_entities_from_folder(const vector_t<editor_entity_payload_t>& entities)
	{
		if (entities.empty())
			return false;

		const world_handle_t		  main_world = editor_world_controller_t::get().get_main_world();
		world_t&					  world		 = editor_world_controller_t::get().get_world(main_world);
		frame_vector_t<entity_guid_t> moved_guids;
		moved_guids.reserve(entities.size());
		for (const editor_entity_payload_t& payload_entity : entities)
		{
			if (!(payload_entity.world == main_world) || payload_entity.entity == NULL_ENTITY_ID || !world.is_alive(payload_entity.entity))
				return false;
			moved_guids.push_back(world.get_entity_guid(payload_entity.entity));
		}

		if (!editor_commands_world_metadata_t::deassign_entities_from_folder({.data = moved_guids.data(), .size = moved_guids.size()}))
			return false;

		refresh_entities();
		return true;
	}

	bool editor_panel_entities_t::assign_payload_folder_to_folder(editor_world_folder_handle_t folder, editor_world_folder_handle_t parent)
	{
		editor_world_metadata_t& metadata = editor_world_metadata_t::get();
		if (!metadata.can_assign_folder(folder, parent))
			return false;

		if (!editor_commands_world_metadata_t::assign_folder_to_folder(folder, parent))
			return false;

		if (!parent.is_null())
			metadata.set_folder_folded(parent, false);
		refresh_entities();
		return true;
	}

	void editor_panel_entities_t::toggle_entity_disabled(entity_id_t entity)
	{
		const world_handle_t main_world = editor_world_controller_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		world_t&				 world			= editor_world_controller_t::get().get_world(main_world);
		world_component_table_t* disabled_table = world.get_component_table(type_id_t<component_disabled_t>::value);
		SFG_ASSERT(disabled_table != nullptr);

		const sid_t component_type = type_id_t<component_disabled_t>::value;
		if (ecs_t::table_has(disabled_table->table, entity))
			editor_commands_component_t::remove(main_world, entity, component_type);
		else
			editor_commands_component_t::add(main_world, entity, component_type);
		refresh_entities();
	}

	void editor_panel_entities_t::start_entity_payload(entity_id_t entity)
	{
		collect_payload_entities(entity);
		if (_payload_entities.empty())
			return;

		editor_payload_controller_t& payload_controller = editor_payload_controller_t::get();
		if (payload_controller.is_payload_active())
			return;

		if (_payload_entities.size() == 1)
		{
			_payload_entity		 = _payload_entities.front();
			const world_t& world = editor_world_controller_t::get().get_world(_payload_entity.world);
			const char*	   name	 = world.get_entity_name(_payload_entity.entity);
			payload_controller.create_payload(name != nullptr ? name : "Entity", editor_payload_type_e::entity, &_payload_entity);
			return;
		}

		string_t text = std::to_string(_payload_entities.size());
		text += " entities";
		payload_controller.create_payload(text.c_str(), editor_payload_type_e::entity_multi, &_payload_entities);
	}

	void editor_panel_entities_t::start_folder_payload(editor_world_folder_handle_t folder)
	{
		editor_world_metadata_t& metadata = editor_world_metadata_t::get();
		if (!metadata.is_folder_valid(folder))
			return;

		editor_payload_controller_t& payload_controller = editor_payload_controller_t::get();
		if (payload_controller.is_payload_active())
			return;

		_payload_folder = folder;
		payload_controller.create_payload(metadata.get_folder(folder).name, editor_payload_type_e::folder, &_payload_folder);
	}

	bool editor_panel_entities_t::reparent_payload_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent)
	{
		if (!can_reparent_entities(entities, parent))
			return false;

		const world_handle_t		main_world = editor_world_controller_t::get().get_main_world();
		frame_vector_t<entity_id_t> moved_entities;
		moved_entities.reserve(entities.size());
		for (const editor_entity_payload_t& payload_entity : entities)
			moved_entities.push_back(payload_entity.entity);

		if (!editor_commands_entity_t::reparent(main_world, moved_entities, parent))
			return false;

		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
		{
			world_t& world = editor_world_controller_t::get().get_world(main_world);
			editor_world_metadata_t::get().set_entity_folded(world.get_entity_guid(parent), false);
		}
		refresh_entities();
		return true;
	}

	void editor_panel_entities_t::duplicate_selected_entities()
	{
		const world_handle_t main_world = editor_world_controller_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(editor_selection_controller_t::get().get_selected_entities().size != 0);

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		frame_vector_t<entity_id_t> duplicates;
		if (editor_commands_entity_t::duplicate(main_world, entities, duplicates))
		{
			const entity_id_t entity = duplicates.back();
			editor_selection_controller_t::get().issue_entity_selection({.data = &entity, .size = 1}, entity);
		}
		refresh_entities();
	}

	void editor_panel_entities_t::destroy_selected_entities()
	{
		const world_handle_t main_world = editor_world_controller_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(editor_selection_controller_t::get().get_selected_entities().size != 0);

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		editor_selection_controller_t::get().clear_entity_selection();
		editor_commands_entity_t::destroy(main_world, entities);
		refresh_entities();
	}

	void editor_panel_entities_t::open_empty_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity = NULL_ENTITY_ID;
		_action_menu_folder = {};

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ENTITY_EMPTY_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS) / sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_empty_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_entities_t::open_folder_action_menu(const vec2f_t& pos, editor_world_folder_handle_t folder)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity = NULL_ENTITY_ID;
		_action_menu_folder = folder;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = FOLDER_ROW_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(FOLDER_ROW_ACTION_MENU_ROWS) / sizeof(FOLDER_ROW_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_folder_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_entities_t::open_folder_rename_popup(editor_world_folder_handle_t folder)
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_world_metadata_t& metadata = editor_world_metadata_t::get();
		if (!metadata.is_folder_valid(folder))
			return;

		const editor_outliner_row_t* const row = find_row_by_folder(folder);
		if (row == nullptr)
			return;

		const ui::layout_tree_t& tree = _ui->get_tree();

		const ui::layout_out_t& row_out	  = tree.out(row->root);
		const ui::layout_out_t& label_out = tree.out(row->label);
		const f32				width	  = row_out.pos.x + row_out.size.x - label_out.pos.x;

		_edit_folder = folder;
		popup->request_input_popup({
			.closed		 = on_folder_rename_popup_closed,
			.user_data	 = this,
			.text		 = metadata.get_folder(folder).name,
			.placeholder = "Folder",
			.pos		 = {label_out.pos.x, row_out.pos.y},
			.width		 = width,
		});
	}

	void editor_panel_entities_t::open_folder_color_popup(const vec2f_t& pos, editor_world_folder_handle_t folder)
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_world_metadata_t& metadata = editor_world_metadata_t::get();
		if (!metadata.is_folder_valid(folder))
			return;

		_edit_folder				= folder;
		_folder_edit_color			= metadata.get_folder(folder).color;
		_folder_edit_original_color = _folder_edit_color;
		color_t* color_fields		= &_folder_edit_color;
		popup->request_color_wheel_popup({
			.fields			 = {.data = &color_fields, .size = 1},
			.edit_begin		 = on_folder_color_wheel_edit_begin,
			.on_data_changed = on_folder_color_wheel_data_changed,
			.closed			 = on_folder_color_wheel_popup_closed,
			.user_data		 = this,
			.pos			 = pos,
		});
	}

	void editor_panel_entities_t::open_entity_action_menu(const vec2f_t& pos, entity_id_t entity)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity						= entity;
		ENTITY_ROW_ACTION_MENU_ROWS[0].disabled = !is_create_enabled();

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ENTITY_ROW_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ENTITY_ROW_ACTION_MENU_ROWS) / sizeof(ENTITY_ROW_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_entity_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

}
