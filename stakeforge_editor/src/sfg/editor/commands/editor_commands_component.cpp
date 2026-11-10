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
#include "editor/commands/editor_commands_component.hpp"
#include "editor_command_entity_util.hpp"
#include "editor/editor_world_controller.hpp"
#include "editor/world/editor_world.hpp"
#include "editor/editor_command_system.hpp"
#include <sfg/core/data/istream.hpp>
#include <sfg/core/data/ostream.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/engine/engine_runtime.hpp>
#include <sfg/core/runtime/world/ecs.hpp>
#include <sfg/core/runtime/world/world.hpp>
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

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(data_size, alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), data, data_size);
			return handle;
		}

		void initialize_component_data(ecs_component_table_t& table, void* component)
		{
			if (table.get_type_desc().size == 0)
				return;

			reflection_registry_t::get().initialize_type(table.get_type_desc().type_id, component);
		}

		void add_empty_component(ecs_component_table_t& table, entity_id_t entity)
		{
			if (table.has(entity))
				return;

			void* component = table.add(entity);
			initialize_component_data(table, component);
		}

		chunk_handle32_t create_stream_array(editor_command_system_t& system, size_t count)
		{
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
			world_t&								world	 = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t& table = *target_table;

			for (u32 i = 0; i < payload.count; ++i)
			{
				if (table.has(entities[i]))
					table.remove(entities[i]);
			}

			return true;
		}

		bool add_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_add_component_payload_t& payload	 = system.get_payload_as<editor_command_add_component_payload_t>(command);
			world_t&								world	 = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t& table = *target_table;

			for (u32 i = 0; i < payload.count; ++i)
			{
				add_empty_component(table, entities[i]);

				world.scan_for_resources(entities[i], true);
			}

			return true;
		}

		bool add_component_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_add_component_payload_t& payload = system.get_payload_as<editor_command_add_component_payload_t>(command);

			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
			}

			return true;
		}

		bool remove_component_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_remove_component_payload_t& payload	= system.get_payload_as<editor_command_remove_component_payload_t>(command);
			world_t&								   world	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				   entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t&	table	= *target_table;
			const chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(payload.streams);

			return editor_commands_component_t::apply_streams(table, {.data = entities.data(), .size = payload.count}, system.get_aux_data(), {.data = streams, .size = payload.count}, editor_component_stream_mode_e::restore);
		}

		bool remove_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_remove_component_payload_t& payload	= system.get_payload_as<editor_command_remove_component_payload_t>(command);
			world_t&								   world	= editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				   entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t& table = *target_table;

			for (u32 i = 0; i < payload.count; ++i)
			{
				if (table.has(entities[i]))
					table.remove(entities[i]);
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

			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
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
			world_t&								  world	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				  entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t&	table	= *target_table;
			const chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(payload.streams);

			return editor_commands_component_t::apply_streams(table, {.data = entities.data(), .size = payload.count}, system.get_aux_data(), {.data = streams, .size = payload.count}, editor_component_stream_mode_e::restore);
		}

		bool reset_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reset_component_payload_t& payload  = system.get_payload_as<editor_command_reset_component_payload_t>(command);
			world_t&								  world	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				  entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t& table = *target_table;

			for (u32 i = 0; i < payload.count; ++i)
			{
				if (table.has(entities[i]))
					initialize_component_data(table, table.get(entities[i]));
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

			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
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
			world_t&								  world	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				  entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t&	table	= *target_table;
			const chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(payload.old_streams);

			return editor_commands_component_t::apply_streams(table, {.data = entities.data(), .size = payload.count}, system.get_aux_data(), {.data = streams, .size = payload.count}, editor_component_stream_mode_e::restore);
		}

		bool paste_component_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_component_payload_t& payload  = system.get_payload_as<editor_command_paste_component_payload_t>(command);
			world_t&								  world	   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			frame_vector_t<entity_id_t>				  entities = {};

			if (!editor_command_entity_util_t::resolve_entities(world, system.get_aux_data(), payload.entity_guids, entities))
				return false;

			ecs_component_table_t* target_table = world.find_component_table(payload.component_type);

			if (target_table == nullptr)
				return false;

			ecs_component_table_t& table = *target_table;

			return editor_commands_component_t::apply_streams(table, {.data = entities.data(), .size = payload.count}, system.get_aux_data(), {.data = &payload.paste_stream, .size = 1}, editor_component_stream_mode_e::paste);
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

			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
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
			if (table.get_type_desc().size == 0)
				return true;

			chunk_handle32_t* streams = system.get_aux_data().get<chunk_handle32_t>(streams_handle);

			for (size_t i = 0; i < entities.size(); ++i)
			{
				ostream_t	stream	  = {};
				const void* component = table.get(entities[i]);

				if (!reflection_registry_t::get().type_to_stream(table.get_type_desc().type_id, const_cast<void*>(component), nullptr, stream))
				{
					SFG_ERR("failed to serialize component {0} for entity {1}", table.get_type_desc().type_id, entities[i]);
					return false;
				}

				streams[i] = copy_stream_to_aux(system, stream);
			}

			return true;
		}
	}

	bool editor_commands_component_t::apply_streams(ecs_component_table_t& table, span_t<const entity_id_t> entities, chunk_allocator_t& aux_data, span_t<const chunk_handle32_t> streams, editor_component_stream_mode_e mode)
	{
		SFG_ASSERT(streams.size == entities.size || streams.size == 1);

		const ecs_component_type_desc_t& type	= table.get_type_desc();
		const size_t					 stride = (static_cast<size_t>(type.size) + type.alignment - 1) & ~(static_cast<size_t>(type.alignment) - 1);
		u8*								 staged = stride != 0 && entities.size != 0 ? static_cast<u8*>(frame_allocator_tls_t::allocate(stride * entities.size, type.alignment)) : nullptr;

		for (size_t i = 0; i < entities.size; ++i)
		{
			if (!table.has(entities.data[i]))
			{
				if (mode == editor_component_stream_mode_e::edit)
				{
					SFG_ERR("component {0} missing on entity {1}", type.type_id, entities.data[i]);
					return false;
				}

				if (mode == editor_component_stream_mode_e::paste)
					continue;
			}

			if (type.size == 0)
				continue;

			void*				   component = staged + i * stride;
			const chunk_handle32_t handle	 = streams.data[streams.size == 1 ? 0 : i];
			istream_t			   stream(handle ? aux_data.get<u8>(handle) : nullptr, handle.size);

			reflection_registry_t::get().initialize_type(type.type_id, component);

			if (!reflection_registry_t::get().type_from_stream(type.type_id, component, nullptr, stream))
			{
				SFG_ERR("failed to deserialize component {0} for entity {1}", type.type_id, entities.data[i]);
				return false;
			}
		}

		for (size_t i = 0; i < entities.size; ++i)
		{
			if (mode == editor_component_stream_mode_e::paste && !table.has(entities.data[i]))
				continue;

			void* component = mode == editor_component_stream_mode_e::restore ? table.add(entities.data[i]) : table.get(entities.data[i]);

			if (type.size != 0)
				SFG_MEMCPY(component, staged + i * stride, type.size);
		}

		return true;
	}

	bool editor_commands_component_t::add(editor_world_handle_t world, entity_id_t entity, sid_t component_type)
	{
		frame_vector_t<entity_id_t> entities = {};

		entities.push_back(entity);

		return add(world, entities, component_type);
	}

	bool editor_commands_component_t::add(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;

		ecs_component_table_t&		table	 = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		frame_vector_t<entity_id_t> affected = {};

		affected.reserve(entities.size());

		for (entity_id_t entity : entities)
		{
			if (!table.has(entity))
				affected.push_back(entity);
		}

		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_add_component_payload_t payload = {};

		payload.entity_guids   = editor_command_entity_util_t::capture_entities(editor_world_controller_t::get().get_editor_world(world)->get_world(), command_system.get_aux_data(), {.data = affected.data(), .size = affected.size()});
		payload.world		   = world;
		payload.component_type = component_type;
		payload.count		   = static_cast<u32>(affected.size());

		const editor_command_issue_desc_t desc{
			.undo		= add_component_undo,
			.redo		= add_component_redo,
			.cleanup	= add_component_cleanup,
			.debug_name = "Add Component",
			.type		= editor_command_type_e::component_add,
			.world		= world,
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
		frame_vector_t<entity_id_t> entities = {};

		entities.push_back(entity);

		return remove(world, entities, component_type);
	}

	bool editor_commands_component_t::remove(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;

		ecs_component_table_t&		table	 = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);
		frame_vector_t<entity_id_t> affected = {};

		affected.reserve(entities.size());

		for (entity_id_t entity : entities)
		{
			if (table.has(entity))
				affected.push_back(entity);
		}

		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_remove_component_payload_t payload = {};

		payload.streams		   = create_stream_array(command_system, affected.size());
		payload.entity_guids   = editor_command_entity_util_t::capture_entities(editor_world_controller_t::get().get_editor_world(world)->get_world(), command_system.get_aux_data(), {.data = affected.data(), .size = affected.size()});
		payload.world		   = world;
		payload.component_type = component_type;
		payload.count		   = static_cast<u32>(affected.size());

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
			.world		= world,
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
		frame_vector_t<entity_id_t> entities = {};

		entities.push_back(entity);

		return reset(world, entities, component_type);
	}

	bool editor_commands_component_t::reset(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type)
	{
		if (entities.empty())
			return false;

		ecs_component_table_t& table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);

		if (table.get_type_desc().size == 0)
			return true;

		frame_vector_t<entity_id_t> affected = {};

		affected.reserve(entities.size());

		for (entity_id_t entity : entities)
		{
			if (table.has(entity))
				affected.push_back(entity);
		}

		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_reset_component_payload_t payload = {};

		payload.streams		   = create_stream_array(command_system, affected.size());
		payload.entity_guids   = editor_command_entity_util_t::capture_entities(editor_world_controller_t::get().get_editor_world(world)->get_world(), command_system.get_aux_data(), {.data = affected.data(), .size = affected.size()});
		payload.world		   = world;
		payload.component_type = component_type;
		payload.count		   = static_cast<u32>(affected.size());

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
			.world		= world,
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
		frame_vector_t<entity_id_t> entities = {};

		entities.push_back(entity);

		return paste(world, entities, component_type, data, data_size);
	}

	bool editor_commands_component_t::paste(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type, const u8* data, size_t data_size)
	{
		if (entities.empty() || data_size == 0)
			return false;

		ecs_component_table_t& table = editor_world_controller_t::get().get_editor_world(world)->get_world().get_component_table(component_type);

		if (table.get_type_desc().size == 0)
			return true;

		frame_vector_t<entity_id_t> affected = {};

		affected.reserve(entities.size());

		for (entity_id_t entity : entities)
		{
			if (table.has(entity))
				affected.push_back(entity);
		}

		if (affected.empty())
			return true;

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_paste_component_payload_t payload = {};

		payload.old_streams	   = create_stream_array(command_system, affected.size());
		payload.paste_stream   = copy_data_to_aux(command_system, data, data_size);
		payload.entity_guids   = editor_command_entity_util_t::capture_entities(editor_world_controller_t::get().get_editor_world(world)->get_world(), command_system.get_aux_data(), {.data = affected.data(), .size = affected.size()});
		payload.world		   = world;
		payload.component_type = component_type;
		payload.count		   = static_cast<u32>(affected.size());

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
			.world		= world,
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
