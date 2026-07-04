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
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/world.hpp>

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

		chunk_handle32_t copy_entities_to_aux(editor_command_system_t& system, const frame_vector_t<entity_id_t>& entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size(), dst);
			SFG_MEMCPY(dst, entities.data(), sizeof(entity_id_t) * entities.size());
			return handle;
		}

		chunk_handle32_t copy_entity_parents_to_aux(editor_command_system_t& system, const world_t& world, const frame_vector_t<entity_id_t>& entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size(), dst);
			for (size_t i = 0; i < entities.size(); ++i)
				dst[i] = world.get_entity_parent(entities[i]);
			return handle;
		}

		chunk_handle32_t create_entity_array(editor_command_system_t& system, size_t count, entity_id_t value)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(count, dst);
			for (size_t i = 0; i < count; ++i)
				dst[i] = value;
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

		bool create_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_create_entity_payload_t& payload = system.get_payload_as<editor_command_create_entity_payload_t>(command);
			world_t&								world	= editor_app_t::get().get_runtime().get_world(payload.world);
			if (payload.folder_guid != 0 && payload.guid != NULL_ENTITY_GUID)
			{
				const entity_guid_t guid = payload.guid;
				editor_app_t::get().get_world_metadata().deassign_entities_from_folder({.data = &guid, .size = 1});
			}
			world.destroy_entity_tree(payload.entity);
			payload.entity = NULL_ENTITY_ID;
			return true;
		}

		bool create_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_create_entity_payload_t& payload = system.get_payload_as<editor_command_create_entity_payload_t>(command);
			world_t&								world	= editor_app_t::get().get_runtime().get_world(payload.world);
			const entity_id_t						entity	= world.create_entity(payload.name, payload.guid);
			if (payload.parent != NULL_ENTITY_ID)
				world.attach_to(entity, payload.parent);
			payload.guid   = world.get_entity_guid(entity);
			payload.entity = entity;
			if (payload.folder_guid != 0)
			{
				editor_world_metadata_t&		   metadata = editor_app_t::get().get_world_metadata();
				const editor_world_folder_handle_t folder	= metadata.get_folder_handle(payload.folder_guid);
				if (!folder.is_null())
					metadata.assign_entities_to_folder(folder, {.data = &payload.guid, .size = 1});
			}
			return true;
		}

		bool duplicate_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload	= system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			world_t&								   world	= editor_app_t::get().get_runtime().get_world(payload.world);
			entity_id_t*							   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = payload.count; i-- > 0;)
			{
				world.destroy_entity_tree(entities[i]);
				entities[i] = NULL_ENTITY_ID;
			}
			return true;
		}

		bool duplicate_entity_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			free_streams(system, payload.streams, payload.count);
			if (payload.streams)
			{
				system.get_aux_data().free(payload.streams);
				payload.streams = {};
			}
			if (payload.sources)
			{
				system.get_aux_data().free(payload.sources);
				payload.sources = {};
			}
			if (payload.parents)
			{
				system.get_aux_data().free(payload.parents);
				payload.parents = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
			return true;
		}

		bool duplicate_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_duplicate_entity_payload_t& payload	= system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			world_t&								   world	= editor_app_t::get().get_runtime().get_world(payload.world);
			const entity_id_t*						   sources	= system.get_aux_data().get<entity_id_t>(payload.sources);
			const entity_id_t*						   parents	= system.get_aux_data().get<entity_id_t>(payload.parents);
			entity_id_t*							   entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			chunk_handle32_t*						   streams	= system.get_aux_data().get<chunk_handle32_t>(payload.streams);

			for (u32 i = 0; i < payload.count; ++i)
			{
				if (!streams[i])
				{
					ostream_t						  stream;
					frame_vector_t<resource_handle_t> resources;
					world_cooker_t::entity_to_stream(world, sources[i], stream, resources);
					streams[i] = copy_stream_to_aux(system, stream);
				}

				if (!streams[i])
				{
					SFG_ERR("failed to serialize entity for duplicate command: {0}", sources[i]);
					for (u32 j = i; j-- > 0;)
					{
						world.destroy_entity_tree(entities[j]);
						entities[j] = NULL_ENTITY_ID;
					}
					return false;
				}

				istream_t		  stream(system.get_aux_data().get<u8>(streams[i]), streams[i].size);
				const entity_id_t entity = world_cooker_t::entity_from_stream(world, stream, true);
				if (entity == NULL_ENTITY_ID)
				{
					SFG_ERR("failed to recreate duplicated entity from stream");
					for (u32 j = i; j-- > 0;)
					{
						world.destroy_entity_tree(entities[j]);
						entities[j] = NULL_ENTITY_ID;
					}
					return false;
				}

				entities[i] = entity;
				if (parents[i] != NULL_ENTITY_ID)
					world.attach_to(entity, parents[i]);
			}
			return true;
		}

		bool destroy_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload  = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
			world_t&								 world	  = editor_app_t::get().get_runtime().get_world(payload.world);
			entity_id_t*							 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			chunk_handle32_t*						 streams  = system.get_aux_data().get<chunk_handle32_t>(payload.streams);
			for (u32 i = payload.count; i-- > 0;)
			{
				istream_t		  stream(system.get_aux_data().get<u8>(streams[i]), streams[i].size);
				const entity_id_t entity = world_cooker_t::entity_from_stream(world, stream);
				system.get_aux_data().free(streams[i]);
				streams[i]	= {};
				entities[i] = entity;
			}
			return true;
		}

		bool destroy_entity_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
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
			return true;
		}

		bool destroy_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_destroy_entity_payload_t& payload  = system.get_payload_as<editor_command_destroy_entity_payload_t>(command);
			world_t&								 world	  = editor_app_t::get().get_runtime().get_world(payload.world);
			const entity_id_t*						 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			chunk_handle32_t*						 streams  = system.get_aux_data().get<chunk_handle32_t>(payload.streams);
			for (u32 i = 0; i < payload.count; ++i)
			{
				ostream_t						  stream;
				frame_vector_t<resource_handle_t> resources;
				world_cooker_t::entity_to_stream(world, entities[i], stream, resources);
				streams[i] = copy_stream_to_aux(system, stream);
				world.destroy_entity_tree(entities[i]);
			}
			return true;
		}

		bool apply_entity_parents(editor_command_system_t& system, const editor_command_reparent_entity_payload_t& payload, chunk_handle32_t parents_handle)
		{
			world_t&		   world	= editor_app_t::get().get_runtime().get_world(payload.world);
			const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const entity_id_t* parents	= system.get_aux_data().get<entity_id_t>(parents_handle);

			for (u32 i = 0; i < payload.count; ++i)
			{
				const entity_id_t entity = entities[i];
				const entity_id_t parent = parents[i];
				if (!world.is_alive(entity) || (parent != NULL_ENTITY_ID && !world.is_alive(parent)))
					continue;

				if (parent == NULL_ENTITY_ID)
					world.detach(entity);
				else
					world.attach_to(entity, parent);
			}
			return true;
		}

		bool reparent_entity_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_reparent_entity_payload_t& payload = system.get_payload_as<editor_command_reparent_entity_payload_t>(command);
			return apply_entity_parents(system, payload, payload.previous_parents);
		}

		bool reparent_entity_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_reparent_entity_payload_t& payload = system.get_payload_as<editor_command_reparent_entity_payload_t>(command);
			return apply_entity_parents(system, payload, payload.next_parents);
		}

		bool reparent_entity_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reparent_entity_payload_t& payload = system.get_payload_as<editor_command_reparent_entity_payload_t>(command);
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
			if (payload.previous_parents)
			{
				system.get_aux_data().free(payload.previous_parents);
				payload.previous_parents = {};
			}
			if (payload.next_parents)
			{
				system.get_aux_data().free(payload.next_parents);
				payload.next_parents = {};
			}
			return true;
		}
	}

	entity_id_t editor_commands_entity_t::create(world_handle_t world, entity_id_t parent, editor_world_folder_handle_t folder)
	{
		editor_command_create_entity_payload_t payload = {};
		payload.world								   = world;
		payload.parent								   = parent;
		payload.entity								   = NULL_ENTITY_ID;
		payload.folder_guid							   = folder.is_null() ? 0 : editor_app_t::get().get_world_metadata().get_folder(folder).guid;
		const char*	 entity_name					   = "Entity";
		const size_t entity_len						   = std::strlen(entity_name);
		const size_t entity_n						   = entity_len < EDITOR_ENTITY_COMMAND_NAME_SIZE - 1 ? entity_len : EDITOR_ENTITY_COMMAND_NAME_SIZE - 1;
		SFG_MEMCPY(payload.name, entity_name, entity_n);
		payload.name[entity_n] = '\0';

		editor_command_system_t&		  command_system = editor_app_t::get().get_command_system();
		const editor_command_issue_desc_t desc{
			.undo			   = create_entity_undo,
			.redo			   = create_entity_redo,
			.debug_name		   = "Create Entity",
			.type			   = editor_command_type_e::entity_create,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue create entity command");
			return NULL_ENTITY_ID;
		}

		editor_command_t&						command		   = command_system.get_command(handle);
		editor_command_create_entity_payload_t& stored_payload = command_system.get_payload_as<editor_command_create_entity_payload_t>(command);
		return stored_payload.entity;
	}

	entity_id_t editor_commands_entity_t::duplicate(world_handle_t world, entity_id_t entity)
	{
		frame_vector_t<entity_id_t> entities;
		frame_vector_t<entity_id_t> out_entities;
		entities.push_back(entity);
		if (!duplicate(world, entities, out_entities))
			return NULL_ENTITY_ID;
		return out_entities[0];
	}

	bool editor_commands_entity_t::duplicate(world_handle_t world, const frame_vector_t<entity_id_t>& entities, frame_vector_t<entity_id_t>& out_entities)
	{
		out_entities.resize(0);
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_duplicate_entity_payload_t payload = {};
		payload.streams									  = create_stream_array(command_system, entities.size());
		payload.sources									  = copy_entities_to_aux(command_system, entities);
		payload.parents									  = copy_entity_parents_to_aux(command_system, editor_app_t::get().get_runtime().get_world(world), entities);
		payload.entities								  = create_entity_array(command_system, entities.size(), NULL_ENTITY_ID);
		payload.world									  = world;
		payload.count									  = static_cast<u32>(entities.size());

		const editor_command_issue_desc_t desc{
			.undo			   = duplicate_entity_undo,
			.redo			   = duplicate_entity_redo,
			.cleanup		   = duplicate_entity_cleanup,
			.debug_name		   = "Duplicate Entity",
			.type			   = editor_command_type_e::entity_duplicate,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue duplicate entity command");
			return false;
		}

		editor_command_t&						   command		   = command_system.get_command(handle);
		editor_command_duplicate_entity_payload_t& stored_payload  = command_system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
		const entity_id_t*						   stored_entities = command_system.get_aux_data().get<entity_id_t>(stored_payload.entities);
		out_entities.reserve(stored_payload.count);
		for (u32 i = 0; i < stored_payload.count; ++i)
			out_entities.push_back(stored_entities[i]);
		return true;
	}

	bool editor_commands_entity_t::destroy(world_handle_t world, entity_id_t entity)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return destroy(world, entities);
	}

	bool editor_commands_entity_t::destroy(world_handle_t world, const frame_vector_t<entity_id_t>& entities)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_destroy_entity_payload_t payload = {};
		payload.streams									= create_stream_array(command_system, entities.size());
		payload.entities								= copy_entities_to_aux(command_system, entities);
		payload.world									= world;
		payload.count									= static_cast<u32>(entities.size());

		const editor_command_issue_desc_t desc{
			.undo			   = destroy_entity_undo,
			.redo			   = destroy_entity_redo,
			.cleanup		   = destroy_entity_cleanup,
			.debug_name		   = "Destroy Entity",
			.type			   = editor_command_type_e::entity_destroy,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue destroy entity command");
			return false;
		}

		return true;
	}

	bool editor_commands_entity_t::reparent(world_handle_t world, const frame_vector_t<entity_id_t>& entities, entity_id_t parent)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		world_t& world_ref = editor_app_t::get().get_runtime().get_world(world);
		for (entity_id_t entity : entities)
		{
			if (!world_ref.is_alive(entity))
				return false;
			if (parent != NULL_ENTITY_ID && !world_ref.is_alive(parent))
				return false;
			if (entity == parent)
				return false;
			for (entity_id_t cur = parent; cur != NULL_ENTITY_ID; cur = world_ref.get_entity_parent(cur))
			{
				if (cur == entity)
					return false;
			}
		}

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_reparent_entity_payload_t payload = {};
		payload.entities								 = copy_entities_to_aux(command_system, entities);
		payload.previous_parents						 = copy_entity_parents_to_aux(command_system, world_ref, entities);
		payload.next_parents							 = create_entity_array(command_system, entities.size(), parent);
		payload.world									 = world;
		payload.count									 = static_cast<u32>(entities.size());

		const editor_command_issue_desc_t desc{
			.undo			   = reparent_entity_undo,
			.redo			   = reparent_entity_redo,
			.cleanup		   = reparent_entity_cleanup,
			.debug_name		   = "Reparent Entity",
			.type			   = editor_command_type_e::entity_reparent,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue reparent entity command");
			return false;
		}

		return true;
	}
}
