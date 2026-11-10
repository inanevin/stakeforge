/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "editor/world/editor_world_edit_context.hpp"
#include "editor/commands/editor_command_entity_util.hpp"
#include "editor/editor_command_system.hpp"
#include "editor/editor_world_controller.hpp"
#include "editor/world/editor_world.hpp"
#include <sfg/core/io/assert.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/runtime/world/ecs.hpp>
#include <sfg/core/runtime/world/engine_components.hpp>
#include <sfg/core/runtime/world/world.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY 64
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS			  1024
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS 64

	namespace
	{
		struct editor_command_entity_selection_payload_t
		{
			chunk_handle32_t	  previous_entities = {};
			chunk_handle32_t	  next_entities		= {};
			editor_world_handle_t context			= {};
			entity_guid_t		  previous_anchor	= NULL_ENTITY_GUID;
			entity_guid_t		  next_anchor		= NULL_ENTITY_GUID;
		};

		bool on_entity_selection_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_entity_selection_payload_t& payload = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
			editor_command_entity_util_t::restore_selection(payload.context, system.get_aux_data(), payload.previous_entities, payload.previous_anchor);

			return true;
		}

		bool on_entity_selection_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_entity_selection_payload_t& payload = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
			editor_command_entity_util_t::restore_selection(payload.context, system.get_aux_data(), payload.next_entities, payload.next_anchor);

			return true;
		}

		bool on_entity_selection_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_entity_selection_payload_t& payload = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
			if (payload.previous_entities)
			{
				system.get_aux_data().free(payload.previous_entities);
				payload.previous_entities = {};
			}
			if (payload.next_entities)
			{
				system.get_aux_data().free(payload.next_entities);
				payload.next_entities = {};
			}
			return true;
		}
	}

	bool editor_world_edit_context_t::is_entity_mutation_allowed(const world_t& world, entity_id_t entity) const
	{
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		return is_entity_mutation_allowed(prefab_table, entity);
	}

	bool editor_world_edit_context_t::is_entity_child_insertion_allowed(const world_t& world, entity_id_t entity) const
	{
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		return prefab_table.find_as_const<component_prefab_reference_t>(entity) == nullptr;
	}

	bool editor_world_edit_context_t::is_entity_mutation_allowed(const ecs_component_table_t& prefab_table, entity_id_t entity) const
	{
		const component_prefab_reference_t* prefab = prefab_table.find_as_const<component_prefab_reference_t>(entity);
		return prefab == nullptr || prefab->is_root;
	}

	bool editor_world_edit_context_t::is_selected_mutable_root_entity(const world_t& world, const ecs_component_table_t& prefab_table, entity_id_t entity) const
	{
		if (!is_entity_mutation_allowed(prefab_table, entity))
			return false;

		for (entity_id_t parent = world.get_entity_parent(entity); parent != NULL_ENTITY_ID; parent = world.get_entity_parent(parent))
		{
			if (std::find(_selected_entities.begin(), _selected_entities.end(), parent) != _selected_entities.end() && is_entity_mutation_allowed(prefab_table, parent))
				return false;
		}
		return true;
	}

	size_t editor_world_edit_context_t::collect_mutable_entities(const world_t& world, span_t<const entity_id_t> entities, span_t<entity_id_t> out_entities) const
	{
		SFG_ASSERT(out_entities.size >= entities.size);
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		size_t						 count		  = 0;
		for (size_t i = 0; i < entities.size; ++i)
		{
			if (is_entity_mutation_allowed(prefab_table, entities.data[i]))
				out_entities.data[count++] = entities.data[i];
		}
		return count;
	}

	size_t editor_world_edit_context_t::collect_selected_mutable_root_entities(const world_t& world, span_t<entity_id_t> out_entities) const
	{
		SFG_ASSERT(out_entities.size >= _selected_entities.size());
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		size_t						 count		  = 0;
		for (entity_id_t entity : _selected_entities)
		{
			if (is_selected_mutable_root_entity(world, prefab_table, entity))
				out_entities.data[count++] = entity;
		}
		return count;
	}

	entity_id_t editor_world_edit_context_t::get_mutable_entity_anchor(const world_t& world) const
	{
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		if (_entity_anchor != NULL_ENTITY_ID && is_selected_mutable_root_entity(world, prefab_table, _entity_anchor))
			return _entity_anchor;

		for (auto it = _selected_entities.rbegin(); it != _selected_entities.rend(); ++it)
		{
			if (is_selected_mutable_root_entity(world, prefab_table, *it))
				return *it;
		}
		return NULL_ENTITY_ID;
	}

	void editor_world_edit_context_t::issue_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		bool same_selection = _selected_entities.size() == entities.size;

		for (size_t i = 0; same_selection && i < entities.size; ++i)
			same_selection = _selected_entities[i] == entities.data[i];

		if (same_selection && _entity_anchor == anchor)
		{
			notify_selection_listeners();

			return;
		}

		editor_command_system_t& command_system = editor_command_system_t::get();
		const world_t&			 world			= editor_world_controller_t::get().get_editor_world(_world)->get_world();

		editor_command_entity_selection_payload_t payload = {};

		payload.context			= _world;
		payload.previous_anchor = world.get_entity_guid(_entity_anchor);
		payload.next_anchor		= world.get_entity_guid(anchor);

		payload.previous_entities = editor_command_entity_util_t::capture_entities(world, command_system.get_aux_data(), {.data = _selected_entities.data(), .size = _selected_entities.size()});
		payload.next_entities	  = editor_command_entity_util_t::capture_entities(world, command_system.get_aux_data(), entities);

		const editor_command_issue_desc_t desc{
			.undo		= on_entity_selection_undo,
			.redo		= on_entity_selection_redo,
			.cleanup	= on_entity_selection_cleanup,
			.debug_name = "Select Entities",
			.type		= editor_command_type_e::entity_selection,
			.world		= _world,
		};
		command_system.issue_command(desc, payload);
	}

	void editor_world_edit_context_t::apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		_selected_entities.resize(0);
		_selected_entities.reserve(entities.size);
		for (size_t i = 0; i < entities.size; ++i)
			_selected_entities.push_back(entities.data[i]);
		_entity_anchor = anchor;
		++_selection_generation;
		notify_selection_listeners();
	}

	void editor_world_edit_context_t::clear_entity_selection()
	{
		issue_entity_selection({}, NULL_ENTITY_ID);
	}

	editor_selection_listener_handle_t editor_world_edit_context_t::add_selection_listener(editor_selection_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(fn != nullptr);
		const editor_selection_listener_handle_t handle	  = _selection_listeners.emplace();
		editor_selection_listener_t&			 listener = _selection_listeners.get(handle);
		listener.fn										  = fn;
		listener.user_data								  = user_data;
		return handle;
	}

	void editor_world_edit_context_t::remove_selection_listener(editor_selection_listener_handle_t handle)
	{
		if (_selection_listeners.is_valid(handle))
			_selection_listeners.remove(handle);
	}

	void editor_world_edit_context_t::notify_selection_listeners()
	{
		for (auto it = _selection_listeners.begin_handle(); it != _selection_listeners.end_handle(); ++it)
		{
			const editor_selection_listener_handle_t handle	  = *it;
			const editor_selection_listener_t&		 listener = _selection_listeners.get(handle);
			if (listener.fn != nullptr)
				listener.fn(*this, listener.user_data);
		}
	}

}
