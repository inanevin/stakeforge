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

#include "commands/editor_command_component_edit.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "editor_command_system.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		chunk_handle32_t copy_entities_to_aux(editor_command_system_t& system, span_t<const entity_id_t> entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size, dst);
			SFG_MEMCPY(dst, entities.data, sizeof(entity_id_t) * entities.size);
			return handle;
		}

		chunk_handle32_t copy_stream_to_aux(editor_command_system_t& system, const ostream_t& stream)
		{
			if (stream.get_size() == 0)
				return {};

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		chunk_handle32_t copy_streams_to_aux(editor_command_system_t& system, span_t<const ostream_t> streams)
		{
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * streams.size, alignof(chunk_handle32_t));
			chunk_handle32_t*	   dst	  = system.get_aux_data().get<chunk_handle32_t>(handle);
			for (size_t i = 0; i < streams.size; ++i)
				dst[i] = copy_stream_to_aux(system, streams.data[i]);
			return handle;
		}

		void free_streams(editor_command_system_t& system, chunk_handle32_t streams_handle, u32 count)
		{
			if (!streams_handle)
				return;

			chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(streams_handle);
			for (u32 i = 0; i < count; ++i)
			{
				if (streams[i])
				{
					system.get_aux_data().free(streams[i]);
					streams[i] = {};
				}
			}
		}

		void free_component_edit_payload(editor_command_system_t& system, editor_command_component_edit_payload_t& payload)
		{
			free_streams(system, payload.previous_streams, payload.count);
			free_streams(system, payload.post_streams, payload.count);
			if (payload.previous_streams)
			{
				system.get_aux_data().free(payload.previous_streams);
				payload.previous_streams = {};
			}
			if (payload.post_streams)
			{
				system.get_aux_data().free(payload.post_streams);
				payload.post_streams = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
		}

		bool streams_equal(span_t<const ostream_t> a, span_t<const ostream_t> b)
		{
			if (a.size != b.size)
				return false;

			for (size_t i = 0; i < a.size; ++i)
			{
				if (a.data[i].get_size() != b.data[i].get_size())
					return false;
				if (a.data[i].get_size() != 0 && SFG_MEMCMP(a.data[i].get_raw(), b.data[i].get_raw(), a.data[i].get_size()) != 0)
					return false;
			}
			return true;
		}

		void scan_resources(world_t& world, span_t<const entity_id_t> entities)
		{
			for (size_t i = 0; i < entities.size; ++i)
				world.scan_for_resources(entities.data[i], true);
		}

		bool apply_component_streams(editor_command_system_t& system, editor_command_component_edit_payload_t& payload, chunk_handle32_t streams_handle)
		{
			world_t&			   world	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			ecs_component_table_t& table	= world.get_component_table(payload.component_type);
			const entity_id_t*	   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			chunk_handle32_t*	   streams	= system.get_aux_data().get<chunk_handle32_t>(streams_handle);

			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!ecs_t::table_has(table, entities[i]))
				{
					SFG_ERR("component {0} missing on entity {1}", payload.component_type, entities[i]);
					return false;
				}

				void* component = ecs_t::table_get(table, entities[i]);
				if (table.type_desc.size != 0)
					reflection_registry_t::get().initialize_type(table.type_desc.type_id, component);

				istream_t stream(streams[i] ? system.get_aux_data().get<u8>(streams[i]) : nullptr, streams[i].size);
				if (!reflection_registry_t::get().type_from_stream(table.type_desc.type_id, component, nullptr, stream))
				{
					SFG_ERR("failed to apply component edit {0} for entity {1}", payload.component_type, entities[i]);
					return false;
				}
			}

			editor_world_controller_t::get().mark_world_dirty(payload.world);
			return true;
		}

		bool component_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_component_edit_payload_t& payload = system.get_payload_as<editor_command_component_edit_payload_t>(command);
			if (!apply_component_streams(system, payload, payload.previous_streams))
				return false;

			world_t&		   world	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; i++)
				world.mark_entity_teleported(entities[i]);

			scan_resources(world, {.data = entities, .size = payload.count});
			return true;
		}

		bool component_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_component_edit_payload_t& payload = system.get_payload_as<editor_command_component_edit_payload_t>(command);
			if (!apply_component_streams(system, payload, payload.post_streams))
				return false;

			world_t&		   world	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload.entities);

			for (u32 i = 0; i < payload.count; i++)
				world.mark_entity_teleported(entities[i]);

			scan_resources(world, {.data = entities, .size = payload.count});
			return true;
		}

		bool component_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_component_edit_payload_t& payload = system.get_payload_as<editor_command_component_edit_payload_t>(command);
			free_component_edit_payload(system, payload);
			return true;
		}
	}

	bool editor_command_component_edit_t::edit(editor_world_handle_t world, span_t<const entity_id_t> entities, sid_t component_type, span_t<const ostream_t> previous_streams, span_t<const ostream_t> post_streams)
	{
		if (entities.size == 0 || previous_streams.size != entities.size || post_streams.size != entities.size)
			return false;
		if (streams_equal(previous_streams, post_streams))
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_component_edit_payload_t payload = {};
		payload.previous_streams						= copy_streams_to_aux(command_system, previous_streams);
		payload.post_streams							= copy_streams_to_aux(command_system, post_streams);
		payload.entities								= copy_entities_to_aux(command_system, entities);
		payload.world									= world;
		payload.component_type							= component_type;
		payload.count									= static_cast<u32>(entities.size);

		const editor_command_issue_desc_t desc{
			.undo		= component_edit_undo,
			.redo		= component_edit_redo,
			.cleanup	= component_edit_cleanup,
			.debug_name = "Component Edit",
			.type		= editor_command_type_e::component_edit,
			.world		= world,
			.run_redo	= false,
			.notify		= false,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue component edit command");
			free_component_edit_payload(command_system, payload);
			return false;
		}

		editor_world_controller_t::get().mark_world_dirty(payload.world);

		world_t& scan_world = editor_world_controller_t::get().get_editor_world(world)->get_world();
		scan_resources(scan_world, entities);
		return true;
	}
}
