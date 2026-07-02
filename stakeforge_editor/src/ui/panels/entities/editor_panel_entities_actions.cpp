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
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		editor_action_menu_row_desc_t ENTITY_CREATE_ROWS[] = {
			{.text = "Empty Entity", .command = entity_action_menu_create_empty},
		};

		editor_action_menu_row_desc_t ENTITY_EMPTY_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ENTITY_ROW_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
			{.text = "Duplicate Entity", .shortcut = "CTRL+D", .command = entity_action_menu_duplicate},
			{.text = "Delete Entity", .shortcut = "DEL", .command = entity_action_menu_delete},
		};
	}
	void editor_panel_entities_t::create_entity(entity_id_t parent)
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		const entity_id_t entity = editor_commands_entity_t::create(main_world, parent);
		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
			_expanded_entities.push_back(parent);
		editor_app_t::get().get_selection_controller().issue_entity_selection({.data = &entity, .size = 1}, entity);
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
			const world_t& world = editor_app_t::get().get_runtime().get_world(_payload_entity.world);
			const char*	   name	 = world.get_entity_name(_payload_entity.entity);
			payload_controller.create_payload(name != nullptr ? name : "Entity", editor_payload_type_e::entity, &_payload_entity);
			return;
		}

		string_t text = std::to_string(_payload_entities.size());
		text += " entities";
		payload_controller.create_payload(text.c_str(), editor_payload_type_e::entity_multi, &_payload_entities);
	}

	bool editor_panel_entities_t::reparent_payload_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent)
	{
		if (!can_reparent_entities(entities, parent))
			return false;

		const world_handle_t		main_world = editor_app_t::get().get_main_world();
		frame_vector_t<entity_id_t> moved_entities;
		moved_entities.reserve(entities.size());
		for (const editor_entity_payload_t& payload_entity : entities)
			moved_entities.push_back(payload_entity.entity);

		if (!editor_commands_entity_t::reparent(main_world, moved_entities, parent))
			return false;

		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
			_expanded_entities.push_back(parent);
		refresh_entities();
		return true;
	}

	void editor_panel_entities_t::duplicate_selected_entities()
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(editor_app_t::get().get_selection_controller().get_selected_entities().size != 0);

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		frame_vector_t<entity_id_t> duplicates;
		if (editor_commands_entity_t::duplicate(main_world, entities, duplicates))
		{
			const entity_id_t entity = duplicates.back();
			editor_app_t::get().get_selection_controller().issue_entity_selection({.data = &entity, .size = 1}, entity);
		}
		refresh_entities();
	}

	void editor_panel_entities_t::destroy_selected_entities()
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(editor_app_t::get().get_selection_controller().get_selected_entities().size != 0);

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		editor_app_t::get().get_selection_controller().clear_entity_selection();
		editor_commands_entity_t::destroy(main_world, entities);
		refresh_entities();
	}

	void editor_panel_entities_t::open_empty_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity = NULL_ENTITY_ID;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ENTITY_EMPTY_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS) / sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_empty_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
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
