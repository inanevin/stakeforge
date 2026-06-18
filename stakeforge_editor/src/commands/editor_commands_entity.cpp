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
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

#include <cstring>

namespace sfg
{
	namespace
	{
		world_t& get_world(world_handle_t handle)
		{
			SFG_ASSERT(!handle.is_null());
			return editor_app_t::get().get_runtime().get_world(handle);
		}

		chunk_handle32_t copy_stream_to_aux(editor_command_system_t& system, const ostream_t& stream)
		{
			if (stream.get_size() == 0)
				return {};

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool create_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_create_entity_payload_t& payload = system.get_payload_as<editor_command_create_entity_payload_t>(command);
			world_t&								world	= get_world(payload.world);
			world.destroy_entity(payload.entity);
			payload.entity = NULL_ENTITY_ID;
			return true;
		}

		bool create_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_create_entity_payload_t& payload = system.get_payload_as<editor_command_create_entity_payload_t>(command);
			world_t&								world	= get_world(payload.world);
			const entity_id_t						entity	= world.create_entity(payload.name);
			if (payload.parent != NULL_ENTITY_ID)
				world.attach_to(entity, payload.parent);
			payload.entity = entity;
			return true;
		}

		bool duplicate_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			world_t&								   world   = get_world(payload.world);
			world.destroy_entity(payload.entity);
			payload.entity = NULL_ENTITY_ID;
			return true;
		}

		bool duplicate_entity_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			if (payload.stream)
			{
				system.get_aux_data().free(payload.stream);
				payload.stream = {};
			}
			return true;
		}

		bool duplicate_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			world_t&								   world   = get_world(payload.world);
			if (!payload.stream)
			{
				ostream_t stream;
				world.entity_to_stream(payload.source, stream);
				payload.stream = copy_stream_to_aux(system, stream);
			}

			if (!payload.stream)
				return false;

			istream_t		  stream(system.get_aux_data().get<u8>(payload.stream), payload.stream.size);
			const entity_id_t entity = world.entity_from_stream(stream);
			if (entity == NULL_ENTITY_ID)
				return false;

			payload.entity = entity;
			return true;
		}

		bool destroy_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
			world_t&								 world	 = get_world(payload.world);
			istream_t								 stream;
			if (payload.stream)
				stream.open(system.get_aux_data().get<u8>(payload.stream), payload.stream.size);
			world.entity_from_stream(stream);
			if (payload.stream)
			{
				system.get_aux_data().free(payload.stream);
				payload.stream = {};
			}
			return true;
		}

		bool destroy_entity_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
			if (payload.stream)
			{
				system.get_aux_data().free(payload.stream);
				payload.stream = {};
			}
			return true;
		}

		bool destroy_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
			world_t&								 world	 = get_world(payload.world);
			ostream_t								 stream;
			world.entity_to_stream(payload.entity, stream);
			payload.stream = copy_stream_to_aux(system, stream);
			world.destroy_entity(payload.entity);
			return true;
		}
	}

	entity_id_t editor_commands_entity_t::create(world_handle_t world, entity_id_t parent)
	{
		editor_command_create_entity_payload_t payload = {};
		payload.world								   = world;
		payload.parent								   = parent;
		payload.entity								   = NULL_ENTITY_ID;
		const char*	 entity_name					   = "Entity";
		const size_t entity_len						   = std::strlen(entity_name);
		const size_t entity_n						   = entity_len < EDITOR_ENTITY_COMMAND_NAME_SIZE - 1 ? entity_len : EDITOR_ENTITY_COMMAND_NAME_SIZE - 1;
		SFG_MEMCPY(payload.name, entity_name, entity_n);
		payload.name[entity_n] = '\0';

		editor_command_system_t&		  command_system = editor_app_t::get().get_command_system();
		const editor_command_issue_desc_t desc{
			.undo		= create_entity_undo,
			.redo		= create_entity_redo,
			.debug_name = "Create Entity",
			.type		= editor_command_type_e::entity_create,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
			return NULL_ENTITY_ID;

		editor_command_t&						command		   = command_system.get_command(handle);
		editor_command_create_entity_payload_t& stored_payload = command_system.get_payload_as<editor_command_create_entity_payload_t>(command);
		return stored_payload.entity;
	}

	entity_id_t editor_commands_entity_t::duplicate(world_handle_t world, entity_id_t entity)
	{
		editor_command_duplicate_entity_payload_t payload = {};
		payload.world									  = world;
		payload.source									  = entity;
		payload.entity									  = NULL_ENTITY_ID;
		payload.stream									  = {};

		editor_command_system_t&		  command_system = editor_app_t::get().get_command_system();
		const editor_command_issue_desc_t desc{
			.undo		= duplicate_entity_undo,
			.redo		= duplicate_entity_redo,
			.cleanup	= duplicate_entity_cleanup,
			.debug_name = "Duplicate Entity",
			.type		= editor_command_type_e::entity_duplicate,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
			return NULL_ENTITY_ID;

		editor_command_t&						   command		  = command_system.get_command(handle);
		editor_command_duplicate_entity_payload_t& stored_payload = command_system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
		return stored_payload.entity;
	}

	bool editor_commands_entity_t::destroy(world_handle_t world, entity_id_t entity)
	{
		editor_command_destroy_entity_payload_t payload = {};
		payload.world									= world;
		payload.entity									= entity;
		payload.stream									= {};

		editor_command_system_t&		  command_system = editor_app_t::get().get_command_system();
		const editor_command_issue_desc_t desc{
			.undo		= destroy_entity_undo,
			.redo		= destroy_entity_redo,
			.cleanup	= destroy_entity_cleanup,
			.debug_name = "Destroy Entity",
			.type		= editor_command_type_e::entity_destroy,
		};

		return !command_system.issue_command(desc, payload).is_null();
	}
}
