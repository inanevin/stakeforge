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

#include "commands/editor_command_prefab_spawn.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "editor_command_system.hpp"
#include "world_edit/editor_world_edit_context.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		chunk_handle32_t copy_selection_to_aux(editor_command_system_t& system, span_t<const entity_id_t> selection)
		{
			if (selection.size == 0)
				return {};

			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(selection.size, dst);
			SFG_MEMCPY(dst, selection.data, sizeof(entity_id_t) * selection.size);
			return handle;
		}

		void apply_selection(editor_world_handle_t context, span_t<const entity_id_t> entities, entity_id_t anchor)
		{
			editor_world_controller_t::get().get_editor_world(context)->get_edit_context().apply_entity_selection(entities, anchor);
		}

		bool prefab_spawn_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			world.destroy_entity_tree(payload.root);
			payload.root						  = NULL_ENTITY_ID;
			const entity_id_t* previous_selection = payload.previous_selection_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.previous_selection) : nullptr;
			apply_selection(payload.previous_selection_context, {.data = previous_selection, .size = payload.previous_selection_count}, payload.previous_anchor);
			return true;
		}

		bool prefab_spawn_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			payload.root								   = world.spawn_prefab(payload.prefab, {.parent = payload.parent});
			if (payload.root == NULL_ENTITY_ID)
				return false;

			apply_selection(editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context().get_world(), {.data = &payload.root, .size = 1}, payload.root);
			return true;
		}

		bool prefab_spawn_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			if (payload.previous_selection)
			{
				system.get_aux_data().free(payload.previous_selection);
				payload.previous_selection = {};
			}
			return true;
		}
	}

	entity_id_t editor_command_prefab_spawn_t::spawn(editor_world_handle_t world, resource_handle_t prefab, entity_id_t parent)
	{
		editor_command_system_t&		command_system		 = editor_command_system_t::get();
		editor_world_controller_t&		world_controller	 = editor_world_controller_t::get();
		editor_world_edit_context_t&	selection_controller = world_controller.get_editor_world(world_controller.get_main_world_handle())->get_edit_context();
		const span_t<const entity_id_t> selection			 = selection_controller.get_selected_entities();
		SFG_ASSERT(selection.size <= UINT32_MAX);

		editor_command_prefab_spawn_payload_t payload = {
			.world						= world,
			.previous_selection_context = selection_controller.get_world(),
			.prefab						= prefab,
			.previous_selection			= copy_selection_to_aux(command_system, selection),
			.parent						= parent,
			.root						= NULL_ENTITY_ID,
			.previous_anchor			= selection_controller.get_entity_anchor(),
			.previous_selection_count	= static_cast<u32>(selection.size),
		};

		const editor_command_issue_desc_t desc{
			.undo			   = prefab_spawn_undo,
			.redo			   = prefab_spawn_redo,
			.cleanup		   = prefab_spawn_cleanup,
			.debug_name		   = "Spawn Prefab",
			.type			   = editor_command_type_e::prefab_spawn,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue prefab spawn command");
			return NULL_ENTITY_ID;
		}

		editor_command_t&					   command		  = command_system.get_command(handle);
		editor_command_prefab_spawn_payload_t& stored_payload = command_system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
		return stored_payload.root;
	}
}
