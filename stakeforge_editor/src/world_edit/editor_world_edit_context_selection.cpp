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
#include "world_edit/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

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
			entity_id_t			  previous_anchor	= NULL_ENTITY_ID;
			entity_id_t			  next_anchor		= NULL_ENTITY_ID;
			u32					  previous_count	= 0;
			u32					  next_count		= 0;
		};

		bool on_entity_selection_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_entity_selection_payload_t& payload  = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
			const entity_id_t*								 entities = payload.previous_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.previous_entities) : nullptr;
			editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context().apply_entity_selection({.data = entities, .size = payload.previous_count}, payload.previous_anchor);
			return true;
		}

		bool on_entity_selection_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_entity_selection_payload_t& payload  = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
			const entity_id_t*								 entities = payload.next_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.next_entities) : nullptr;
			editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context().apply_entity_selection({.data = entities, .size = payload.next_count}, payload.next_anchor);
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

	void editor_world_edit_context_t::issue_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		SFG_ASSERT(_inited);

		bool same_selection = _selected_entities.size() == entities.size;
		for (size_t i = 0; same_selection && i < entities.size; ++i)
			same_selection = _selected_entities[i] == entities.data[i];
		if (same_selection && _entity_anchor == anchor)
			return;

		SFG_ASSERT(_selected_entities.size() <= UINT32_MAX);
		SFG_ASSERT(entities.size <= UINT32_MAX);

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_entity_selection_payload_t payload = {};
		payload.context									  = _world;
		payload.previous_anchor							  = _entity_anchor;
		payload.next_anchor								  = anchor;
		payload.previous_count							  = static_cast<u32>(_selected_entities.size());
		payload.next_count								  = static_cast<u32>(entities.size);

		if (payload.previous_count != 0)
		{
			entity_id_t* previous	  = nullptr;
			payload.previous_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.previous_count, previous);
			SFG_MEMCPY(previous, _selected_entities.data(), sizeof(entity_id_t) * payload.previous_count);
		}

		if (payload.next_count != 0)
		{
			entity_id_t* next	  = nullptr;
			payload.next_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.next_count, next);
			SFG_MEMCPY(next, entities.data, sizeof(entity_id_t) * payload.next_count);
		}

		const editor_command_issue_desc_t desc{
			.undo		= on_entity_selection_undo,
			.redo		= on_entity_selection_redo,
			.cleanup	= on_entity_selection_cleanup,
			.debug_name = "Select Entities",
			.type		= editor_command_type_e::entity_selection,
		};
		command_system.issue_command(desc, payload);
	}

	void editor_world_edit_context_t::apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		SFG_ASSERT(_inited);

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
		SFG_ASSERT(_inited);
		SFG_ASSERT(fn != nullptr);

		const editor_selection_listener_handle_t handle	  = _selection_listeners.emplace();
		editor_selection_listener_t&			 listener = _selection_listeners.get(handle);
		listener.fn										  = fn;
		listener.user_data								  = user_data;
		return handle;
	}

	void editor_world_edit_context_t::remove_selection_listener(editor_selection_listener_handle_t handle)
	{
		SFG_ASSERT(_inited);
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
