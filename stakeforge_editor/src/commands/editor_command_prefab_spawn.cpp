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
#include "editor_app.hpp"
#include "editor_command_system.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		bool prefab_spawn_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_app_t::get().get_runtime().get_world(payload.world);
			world.destroy_entity_tree(payload.root);
			payload.root = NULL_ENTITY_ID;
			return true;
		}

		bool prefab_spawn_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_app_t::get().get_runtime().get_world(payload.world);
			payload.root								   = world.spawn_prefab(payload.prefab, {.parent = payload.parent});
			return payload.root != NULL_ENTITY_ID;
		}
	}

	entity_id_t editor_command_prefab_spawn_t::spawn(world_handle_t world, resource_handle_t prefab, entity_id_t parent)
	{
		editor_command_prefab_spawn_payload_t payload = {
			.world	= world,
			.prefab = prefab,
			.parent = parent,
			.root	= NULL_ENTITY_ID,
		};

		editor_command_system_t&		  command_system = editor_app_t::get().get_command_system();
		const editor_command_issue_desc_t desc{
			.undo			   = prefab_spawn_undo,
			.redo			   = prefab_spawn_redo,
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
