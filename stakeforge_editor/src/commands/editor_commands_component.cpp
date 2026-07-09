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
#include "commands/editor_commands_component.hpp"
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
#include <limits>

namespace sfg
{
	namespace
	{
		chunk_handle32_t copy_stream_to_aux(editor_command_system_t& system, const ostream_t& stream)
		{
			if (stream.get_size() == 0)
				return {};

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		chunk_handle32_t copy_data_to_aux(editor_command_system_t& system, const u8* data, size_t data_size)
		{
			if (data_size == 0)
				return {};

			SFG_ASSERT(data != nullptr);
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(data_size, alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), data, data_size);
			return handle;
		}

		void initialize_component_data(ecs_component_table_t& table, void* component)
		{
			if (table.type_desc.size == 0)
				return;

			SFG_ASSERT(component != nullptr);
			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(table.type_desc.type_id);
			SFG_ASSERT(reflected_type != nullptr && reflected_type->default_init_fn != nullptr);
			reflected_type->default_init_fn(component);
		}

		void add_empty_component(ecs_component_table_t& table, entity_id_t entity)
		{
			if (ecs_t::table_has(table, entity))
				return;

			void* component = ecs_t::table_add(table, entity);
			initialize_component_data(table, component);
		}

		bool restore_component(ecs_component_table_t& table, entity_id_t entity, chunk_allocator_t& aux_data, chunk_handle32_t stream_handle)
		{
			void* component = ecs_t::table_has(table, entity) ? ecs_t::table_get(table, entity) : ecs_t::table_add(table, entity);
			if (table.type_desc.size == 0)
				return true;

			initialize_component_data(table, component);
			istream_t stream(stream_handle ? aux_data.get<u8>(stream_handle) : nullptr, stream_handle.size);
			if (!reflection_registry_t::get().type_from_stream(table.type_desc.type_id, component, nullptr, stream))
			{
				SFG_ERR("failed to restore component {0} for entity {1}", table.type_desc.type_id, entity);
				return false;
			}

			return true;
		}

		bool paste_component_data(ecs_component_table_t& table, entity_id_t entity, chunk_allocator_t& aux_data, chunk_handle32_t stream_handle)
		{
			if (!ecs_t::table_has(table, entity))
				return true;
			if (table.type_desc.size == 0)
				return true;

			void* component = ecs_t::table_get(table, entity);
			initialize_component_data(table, component);
			istream_t stream(aux_data.get<u8>(stream_handle), stream_handle.size);
			if (!reflection_registry_t::get().type_from_stream(table.type_desc.type_id, component, nullptr, stream))
			{
				SFG_ERR("failed to paste component {0} for entity {1}", table.type_desc.type_id, entity);
				return false;
			}

			return true;
		}

		chunk_handle32_t create_entity_array(editor_command_system_t& system, const frame_vector_t<entity_id_t>& entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size(), dst);
			SFG_MEMCPY(dst, entities.data(), sizeof(entity_id_t) * entities.size());
			return handle;
		}

		chunk_handle32_t create_stream_array(editor_command_system_t& system, size_t count)
		{
			SFG_ASSERT(count <= std::numeric_limits<size_t>::max() / sizeof(chunk_handle32_t));
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * count, alignof(chunk_handle32_t));
			SFG_MEMSET(system.get_aux_data().get<chunk_handle32_t>(handle), 0, handle.size);
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

		bool add_component_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_add_component_payload_t& payload	 = system.get_payload_as<editor_command_add_component_payload_t>(command);
			ecs_component_table_t&					table	 = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (ecs_t::table_has(table, entities[i]))
					ecs_t::table_remove(table, entities[i]);
			}
			return true;
		}

		bool add_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_add_component_payload_t& payload	 = system.get_payload_as<editor_command_add_component_payload_t>(command);
			ecs_component_table_t&					table	 = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
				add_empty_component(table, entities[i]);
			return true;
		}

		bool add_component_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_add_component_payload_t& payload = system.get_payload_as<editor_command_add_component_payload_t>(command);
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
			return true;
		}

		bool remove_component_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_remove_component_payload_t& payload	= system.get_payload_as<editor_command_remove_component_payload_t>(command);
			ecs_component_table_t&					   table	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const chunk_handle32_t*					   streams	= system.get_aux_data().get<chunk_handle32_t>(payload.streams);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!restore_component(table, entities[i], system.get_aux_data(), streams[i]))
				{
					SFG_ERR("failed to undo remove component command for entity {0}", entities[i]);
					return false;
				}
			}
			return true;
		}

		bool remove_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_remove_component_payload_t& payload	= system.get_payload_as<editor_command_remove_component_payload_t>(command);
			ecs_component_table_t&					   table	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (ecs_t::table_has(table, entities[i]))
					ecs_t::table_remove(table, entities[i]);
			}
			return true;
		}

		void free_remove_component_payload(editor_command_system_t& system, editor_command_remove_component_payload_t& payload)
		{
			free_streams(system, payload.streams, payload.count);
			if (payload.streams)
			{
				system.get_aux_data().free(payload.streams);
				payload.streams = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
		}

		bool remove_component_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_remove_component_payload_t& payload = system.get_payload_as<editor_command_remove_component_payload_t>(command);
			free_remove_component_payload(system, payload);
			return true;
		}

		bool reset_component_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reset_component_payload_t& payload  = system.get_payload_as<editor_command_reset_component_payload_t>(command);
			ecs_component_table_t&					  table	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const chunk_handle32_t*					  streams  = system.get_aux_data().get<chunk_handle32_t>(payload.streams);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!restore_component(table, entities[i], system.get_aux_data(), streams[i]))
				{
					SFG_ERR("failed to undo reset component command for entity {0}", entities[i]);
					return false;
				}
			}
			return true;
		}

		bool reset_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reset_component_payload_t& payload  = system.get_payload_as<editor_command_reset_component_payload_t>(command);
			ecs_component_table_t&					  table	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (ecs_t::table_has(table, entities[i]))
					initialize_component_data(table, ecs_t::table_get(table, entities[i]));
			}
			return true;
		}

		void free_reset_component_payload(editor_command_system_t& system, editor_command_reset_component_payload_t& payload)
		{
			free_streams(system, payload.streams, payload.count);
			if (payload.streams)
			{
				system.get_aux_data().free(payload.streams);
				payload.streams = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
		}

		bool reset_component_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reset_component_payload_t& payload = system.get_payload_as<editor_command_reset_component_payload_t>(command);
			free_reset_component_payload(system, payload);
			return true;
		}

		bool paste_component_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_component_payload_t& payload  = system.get_payload_as<editor_command_paste_component_payload_t>(command);
			ecs_component_table_t&					  table	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const chunk_handle32_t*					  streams  = system.get_aux_data().get<chunk_handle32_t>(payload.old_streams);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!restore_component(table, entities[i], system.get_aux_data(), streams[i]))
				{
					SFG_ERR("failed to undo paste component command for entity {0}", entities[i]);
					return false;
				}
			}
			return true;
		}

		bool paste_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_component_payload_t& payload  = system.get_payload_as<editor_command_paste_component_payload_t>(command);
			ecs_component_table_t&					  table	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world().get_component_table(payload.component_type);
			const entity_id_t*						  entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!paste_component_data(table, entities[i], system.get_aux_data(), payload.paste_stream))
				{
					SFG_ERR("failed to redo paste component command for entity {0}", entities[i]);
					return false;
				}
			}
			return true;
		}

		void free_paste_component_payload(editor_command_system_t& system, editor_command_paste_component_payload_t& payload)
		{
			free_streams(system, payload.old_streams, payload.count);
			if (payload.old_streams)
			{
				system.get_aux_data().free(payload.old_streams);
				payload.old_streams = {};
			}
			if (payload.paste_stream)
			{
				system.get_aux_data().free(payload.paste_stream);
				payload.paste_stream = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
		}

		bool paste_component_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_component_payload_t& payload = system.get_payload_as<editor_command_paste_component_payload_t>(command);
			free_paste_component_payload(system, payload);
			return true;
		}

		bool serialize_removed_components(editor_command_system_t& system, const ecs_component_table_t& table, const frame_vector_t<entity_id_t>& entities, chunk_handle32_t streams_handle)
		{
			if (table.type_desc.size == 0)
				return true;

			chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(streams_handle);
			for (size_t i = 0; i < entities.size(); ++i)
			{
				ostream_t	stream;
				const void* component = ecs_t::table_get(table, entities[i]);
				if (!reflection_registry_t::get().type_to_stream(table.type_desc.type_id, const_cast<void*>(component), nullptr, stream))
				{
					SFG_ERR("failed to serialize component {0} for entity {1}", table.type_desc.type_id, entities[i]);
					return false;
				}
				streams[i] = copy_stream_to_aux(system, stream);
			}
			return true;
		}
	}

	bool editor_commands_component_t::add(editor_world_handle_t world, entity_id_t entity, sid_t component_type)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return add(world, entities, component_type);
	}

	bool editor_commands_component_t::add(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		ecs_component_table_t&		table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		frame_vector_t<entity_id_t> affected;
		affected.reserve(entities.size());
		for (entity_id_t entity : entities)
		{
			if (!ecs_t::table_has(table, entity))
				affected.push_back(entity);
		}
		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_add_component_payload_t payload = {};
		payload.entities							   = create_entity_array(command_system, affected);
		payload.world								   = world;
		payload.component_type						   = component_type;
		payload.count								   = static_cast<u32>(affected.size());

		const editor_command_issue_desc_t desc{
			.undo		= add_component_undo,
			.redo		= add_component_redo,
			.cleanup	= add_component_cleanup,
			.debug_name = "Add Component",
			.type		= editor_command_type_e::component_add,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue add component command");
			return false;
		}

		return true;
	}

	bool editor_commands_component_t::remove(editor_world_handle_t world, entity_id_t entity, sid_t component_type)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return remove(world, entities, component_type);
	}

	bool editor_commands_component_t::remove(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		ecs_component_table_t&		table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		frame_vector_t<entity_id_t> affected;
		affected.reserve(entities.size());
		for (entity_id_t entity : entities)
		{
			if (ecs_t::table_has(table, entity))
				affected.push_back(entity);
		}
		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_remove_component_payload_t payload = {};
		payload.streams									  = create_stream_array(command_system, affected.size());
		payload.entities								  = create_entity_array(command_system, affected);
		payload.world									  = world;
		payload.component_type							  = component_type;
		payload.count									  = static_cast<u32>(affected.size());

		if (!serialize_removed_components(command_system, table, affected, payload.streams))
		{
			SFG_ERR("failed to serialize components for remove command");
			free_remove_component_payload(command_system, payload);
			return false;
		}

		const editor_command_issue_desc_t desc{
			.undo		= remove_component_undo,
			.redo		= remove_component_redo,
			.cleanup	= remove_component_cleanup,
			.debug_name = "Remove Component",
			.type		= editor_command_type_e::component_remove,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue remove component command");
			return false;
		}

		return true;
	}

	bool editor_commands_component_t::reset(editor_world_handle_t world, entity_id_t entity, sid_t component_type)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return reset(world, entities, component_type);
	}

	bool editor_commands_component_t::reset(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		ecs_component_table_t& table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		if (table.type_desc.size == 0)
			return true;

		frame_vector_t<entity_id_t> affected;
		affected.reserve(entities.size());
		for (entity_id_t entity : entities)
		{
			if (ecs_t::table_has(table, entity))
				affected.push_back(entity);
		}
		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_reset_component_payload_t payload = {};
		payload.streams									 = create_stream_array(command_system, affected.size());
		payload.entities								 = create_entity_array(command_system, affected);
		payload.world									 = world;
		payload.component_type							 = component_type;
		payload.count									 = static_cast<u32>(affected.size());

		if (!serialize_removed_components(command_system, table, affected, payload.streams))
		{
			SFG_ERR("failed to serialize components for reset command");
			free_reset_component_payload(command_system, payload);
			return false;
		}

		const editor_command_issue_desc_t desc{
			.undo		= reset_component_undo,
			.redo		= reset_component_redo,
			.cleanup	= reset_component_cleanup,
			.debug_name = "Reset Component",
			.type		= editor_command_type_e::component_reset,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue reset component command");
			return false;
		}

		return true;
	}

	bool editor_commands_component_t::paste(editor_world_handle_t world, entity_id_t entity, sid_t component_type, const u8* data, size_t data_size)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return paste(world, entities, component_type, data, data_size);
	}

	bool editor_commands_component_t::paste(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type, const u8* data, size_t data_size)
	{
		if (entities.empty() || data_size == 0)
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);
		SFG_ASSERT(data != nullptr);

		ecs_component_table_t& table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		if (table.type_desc.size == 0)
			return true;

		frame_vector_t<entity_id_t> affected;
		affected.reserve(entities.size());
		for (entity_id_t entity : entities)
		{
			if (ecs_t::table_has(table, entity))
				affected.push_back(entity);
		}
		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_paste_component_payload_t payload = {};
		payload.old_streams								 = create_stream_array(command_system, affected.size());
		payload.paste_stream							 = copy_data_to_aux(command_system, data, data_size);
		payload.entities								 = create_entity_array(command_system, affected);
		payload.world									 = world;
		payload.component_type							 = component_type;
		payload.count									 = static_cast<u32>(affected.size());

		if (!serialize_removed_components(command_system, table, affected, payload.old_streams))
		{
			SFG_ERR("failed to serialize components for paste command");
			free_paste_component_payload(command_system, payload);
			return false;
		}

		const editor_command_issue_desc_t desc{
			.undo		= paste_component_undo,
			.redo		= paste_component_redo,
			.cleanup	= paste_component_cleanup,
			.debug_name = "Paste Component",
			.type		= editor_command_type_e::component_paste,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue paste component command");
			return false;
		}

		return true;
	}
}
