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
#include "commands/editor_commands_reflection.hpp"
#include "editor_app.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

#include <cstring>

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

		chunk_handle32_t copy_text_to_aux(editor_command_system_t& system, const char* text)
		{
			const char*			   src		 = text != nullptr ? text : "";
			const size_t		   text_size = std::strlen(src) + 1;
			const chunk_handle32_t handle	 = system.get_aux_data().allocate_bytes(text_size, alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<char>(handle), src, text_size);
			return handle;
		}

		chunk_handle32_t copy_entities_to_aux(editor_command_system_t& system, const entity_id_t* entities, u32 entity_count)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entity_count, dst);
			SFG_MEMCPY(dst, entities, sizeof(entity_id_t) * entity_count);
			return handle;
		}

		chunk_handle32_t allocate_chunk_handle_array(editor_command_system_t& system, u32 count, chunk_handle32_t*& out)
		{
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * count, alignof(chunk_handle32_t));
			out							  = system.get_aux_data().get<chunk_handle32_t>(handle);
			SFG_MEMSET(out, 0, sizeof(chunk_handle32_t) * count);
			return handle;
		}

		void* resolve_world_component_object(world_t& world, sid_t type_id, entity_id_t entity)
		{
			world_component_table_t* table = world.find_component_table(type_id);
			if (table == nullptr || !ecs_t::table_has(table->table, entity))
				return nullptr;
			return ecs_t::table_get(table->table, entity);
		}

		void* resolve_target_object(editor_command_system_t& system, const editor_command_reflected_field_edit_payload_t& payload)
		{
			switch (payload.target.kind)
			{
			case editor_reflected_edit_target_kind_e::raw_object:
				if (!payload.target.required_listener.is_null() && !system.is_listener_valid(payload.target.required_listener))
					return nullptr;
				return payload.target.object;
			case editor_reflected_edit_target_kind_e::world_component: {
				world_t& world = editor_app_t::get().get_runtime().get_world(payload.target.world);
				return resolve_world_component_object(world, payload.target.type_id, payload.target.entity);
			}
			default:
				return nullptr;
			}
		}

		bool serialize_builtin_component_field_to_stream(sid_t type_id, sid_t field_id, const void* object, ostream_t& stream)
		{
			if (type_id == component_transform_t::TYPE_ID)
			{
				const component_transform_t& transform = *static_cast<const component_transform_t*>(object);
				if (field_id == "pos"_hs)
				{
					stream << transform.pos;
					return true;
				}
				if (field_id == "rot"_hs)
				{
					stream << transform.rot;
					return true;
				}
				SFG_ASSERT(field_id == "scale"_hs);
				stream << transform.scale;
				return true;
			}

			if (type_id == component_name_t::TYPE_ID)
			{
				const component_name_t& name = *static_cast<const component_name_t*>(object);
				SFG_ASSERT(field_id == "text_index"_hs);
				stream << name.text_index;
				return true;
			}

			return false;
		}

		bool deserialize_builtin_component_field_from_stream(sid_t type_id, sid_t field_id, void* object, istream_t& stream)
		{
			if (type_id == component_transform_t::TYPE_ID)
			{
				component_transform_t& transform = *static_cast<component_transform_t*>(object);
				if (field_id == "pos"_hs)
				{
					stream >> transform.pos;
					return true;
				}
				if (field_id == "rot"_hs)
				{
					stream >> transform.rot;
					return true;
				}
				SFG_ASSERT(field_id == "scale"_hs);
				stream >> transform.scale;
				return true;
			}

			if (type_id == component_name_t::TYPE_ID)
			{
				component_name_t& name = *static_cast<component_name_t*>(object);
				SFG_ASSERT(field_id == "text_index"_hs);
				stream >> name.text_index;
				return true;
			}

			return false;
		}

		bool read_text_id_field(const editor_command_reflected_field_edit_payload_t& payload, void* object, u32& out_text_id)
		{
			ostream_t stream;
			if (!serialize_builtin_component_field_to_stream(payload.type_id, payload.field_id, object, stream) && !reflection_registry_t::get().serialize_field_to_stream(payload.type_id, payload.field_id, object, stream))
			{
				SFG_ERR("failed to serialize reflected text field {0}", payload.field_id);
				return false;
			}
			istream_t input(stream.get_raw(), stream.get_size());
			input >> out_text_id;
			return true;
		}

		bool write_text_id_field(const editor_command_reflected_field_edit_payload_t& payload, void* object, u32 text_id)
		{
			ostream_t stream;
			stream << text_id;
			istream_t input(stream.get_raw(), stream.get_size());
			return deserialize_builtin_component_field_from_stream(payload.type_id, payload.field_id, object, input) || reflection_registry_t::get().deserialize_field_from_stream(payload.type_id, payload.field_id, object, input);
		}

		bool apply_text_id_to_object(editor_command_system_t& system, editor_command_reflected_field_edit_payload_t& payload, world_t& world, void* object, chunk_handle32_t value)
		{
			u32 current_text_id = ECS_INVALID_INDEX;
			if (!read_text_id_field(payload, object, current_text_id))
			{
				SFG_ERR("failed to read reflected text id field {0}", payload.field_id);
				return false;
			}

			world.release_text(current_text_id);
			const char* text		= system.get_aux_data().get<char>(value);
			const u32	new_text_id = world.allocate_text(text != nullptr ? text : "");
			if (!write_text_id_field(payload, object, new_text_id))
			{
				SFG_ERR("failed to write reflected text id field {0}", payload.field_id);
				return false;
			}

			return true;
		}

		bool apply_text_id_value(editor_command_system_t& system, editor_command_reflected_field_edit_payload_t& payload, chunk_handle32_t value, bool multi_old_value)
		{
			world_t& world = editor_app_t::get().get_runtime().get_world(payload.world);
			if (payload.target.kind == editor_reflected_edit_target_kind_e::world_components)
			{
				const entity_id_t* entities	  = system.get_aux_data().get<entity_id_t>(payload.entities);
				chunk_handle32_t*  old_values = payload.old_values ? system.get_aux_data().get<chunk_handle32_t>(payload.old_values) : nullptr;
				for (u32 i = 0; i < payload.entity_count; ++i)
				{
					void* object = resolve_world_component_object(world, payload.target.type_id, entities[i]);
					if (object == nullptr)
						continue;
					if (!apply_text_id_to_object(system, payload, world, object, multi_old_value ? old_values[i] : value))
					{
						SFG_ERR("failed to apply reflected text id field {0} to entity {1}", payload.field_id, entities[i]);
						return false;
					}
				}
				return true;
			}

			void* object = resolve_target_object(system, payload);
			if (object == nullptr)
				return true;
			return apply_text_id_to_object(system, payload, world, object, value);
		}

		bool apply_value(editor_command_system_t& system, editor_command_reflected_field_edit_payload_t& payload, chunk_handle32_t value)
		{
			if (payload.text_id)
				return apply_text_id_value(system, payload, value, value.head == payload.old_value.head && value.size == payload.old_value.size);

			if (payload.target.kind == editor_reflected_edit_target_kind_e::world_components)
			{
				world_t&				world	   = editor_app_t::get().get_runtime().get_world(payload.target.world);
				const entity_id_t*		entities   = system.get_aux_data().get<entity_id_t>(payload.entities);
				const chunk_handle32_t* old_values = payload.old_values ? system.get_aux_data().get<chunk_handle32_t>(payload.old_values) : nullptr;
				const bool				undo	   = value.head == payload.old_value.head && value.size == payload.old_value.size;
				for (u32 i = 0; i < payload.entity_count; ++i)
				{
					void* object = resolve_world_component_object(world, payload.target.type_id, entities[i]);
					if (object == nullptr)
						continue;
					const chunk_handle32_t entity_value = undo ? old_values[i] : value;
					istream_t			   stream(system.get_aux_data().get<u8>(entity_value), entity_value.size);
					if (!deserialize_builtin_component_field_from_stream(payload.type_id, payload.field_id, object, stream) && !reflection_registry_t::get().deserialize_field_from_stream(payload.type_id, payload.field_id, object, stream))
					{
						SFG_ERR("failed to apply reflected field {0} to entity {1}", payload.field_id, entities[i]);
						return false;
					}
				}
				return true;
			}

			void* object = resolve_target_object(system, payload);
			if (object == nullptr)
				return true;

			istream_t stream(system.get_aux_data().get<u8>(value), value.size);
			if (!deserialize_builtin_component_field_from_stream(payload.type_id, payload.field_id, object, stream) && !reflection_registry_t::get().deserialize_field_from_stream(payload.type_id, payload.field_id, object, stream))
			{
				SFG_ERR("failed to apply reflected field {0}", payload.field_id);
				return false;
			}

			return true;
		}

		bool reflected_field_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reflected_field_edit_payload_t& payload = system.get_payload_as<editor_command_reflected_field_edit_payload_t>(command);
			return apply_value(system, payload, payload.old_value);
		}

		bool reflected_field_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reflected_field_edit_payload_t& payload = system.get_payload_as<editor_command_reflected_field_edit_payload_t>(command);
			return apply_value(system, payload, payload.new_value);
		}

		bool reflected_field_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_reflected_field_edit_payload_t& payload = system.get_payload_as<editor_command_reflected_field_edit_payload_t>(command);
			if (payload.old_value)
			{
				system.get_aux_data().free(payload.old_value);
				payload.old_value = {};
			}
			if (payload.new_value)
			{
				system.get_aux_data().free(payload.new_value);
				payload.new_value = {};
			}
			if (payload.old_values)
			{
				chunk_handle32_t* values = system.get_aux_data().get<chunk_handle32_t>(payload.old_values);
				for (u32 i = 0; i < payload.entity_count; ++i)
				{
					if (values[i])
						system.get_aux_data().free(values[i]);
				}
				system.get_aux_data().free(payload.old_values);
				payload.old_values = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
			return true;
		}
	}

	bool editor_commands_reflection_t::edit_field(const editor_reflected_field_edit_desc_t& desc, const ostream_t& old_value, const ostream_t& new_value)
	{
		if (desc.target.kind != editor_reflected_edit_target_kind_e::world_components && old_value.get_size() == new_value.get_size() && (old_value.get_size() == 0 || std::memcmp(old_value.get_raw(), new_value.get_raw(), old_value.get_size()) == 0))
			return true;

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_reflected_field_edit_payload_t payload = {};
		payload.old_value									  = copy_stream_to_aux(command_system, old_value);
		payload.new_value									  = copy_stream_to_aux(command_system, new_value);
		payload.target										  = desc.target;
		payload.type_id										  = desc.type_id;
		payload.field_id									  = desc.field_id;
		bool run_redo										  = false;

		if (desc.target.kind == editor_reflected_edit_target_kind_e::world_components)
		{
			SFG_ASSERT(desc.target.entities != nullptr);
			SFG_ASSERT(desc.target.entity_count != 0);
			payload.entities			 = copy_entities_to_aux(command_system, desc.target.entities, desc.target.entity_count);
			payload.entity_count		 = desc.target.entity_count;
			payload.target.entities		 = nullptr;
			payload.target.entity_count	 = desc.target.entity_count;
			chunk_handle32_t* old_values = nullptr;
			payload.old_values			 = allocate_chunk_handle_array(command_system, desc.target.entity_count, old_values);
			old_values[0]				 = copy_stream_to_aux(command_system, old_value);
			world_t& world				 = editor_app_t::get().get_runtime().get_world(desc.target.world);
			for (u32 i = 1; i < desc.target.entity_count; ++i)
			{
				void* object = resolve_world_component_object(world, desc.target.type_id, desc.target.entities[i]);
				SFG_ASSERT(object != nullptr);
				ostream_t  entity_old_value;
				const bool serialized = serialize_builtin_component_field_to_stream(desc.type_id, desc.field_id, object, entity_old_value) || reflection_registry_t::get().serialize_field_to_stream(desc.type_id, desc.field_id, object, entity_old_value);
				SFG_ASSERT(serialized);
				if (!serialized)
				{
					SFG_ERR("failed to serialize reflected field {0} for entity {1}", desc.field_id, desc.target.entities[i]);
					return false;
				}
				old_values[i] = copy_stream_to_aux(command_system, entity_old_value);
			}
			run_redo = true;
		}

		const editor_command_issue_desc_t issue_desc{
			.undo		= reflected_field_edit_undo,
			.redo		= reflected_field_edit_redo,
			.cleanup	= reflected_field_edit_cleanup,
			.debug_name = "Edit Reflected Field",
			.type		= editor_command_type_e::reflected_field_edit,
			.run_redo	= run_redo,
		};

		const editor_command_handle_t handle = command_system.issue_command(issue_desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue reflected field edit command");
			return false;
		}

		return true;
	}

	bool editor_commands_reflection_t::edit_text_id_field(const editor_reflected_field_edit_desc_t& desc, world_handle_t world, const char* old_value, const char* new_value)
	{
		const char* old_text = old_value != nullptr ? old_value : "";
		const char* new_text = new_value != nullptr ? new_value : "";
		if (desc.target.kind != editor_reflected_edit_target_kind_e::world_components && std::strcmp(old_text, new_text) == 0)
			return true;

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_reflected_field_edit_payload_t payload = {};
		payload.old_value									  = copy_text_to_aux(command_system, old_text);
		payload.new_value									  = copy_text_to_aux(command_system, new_text);
		payload.target										  = desc.target;
		payload.world										  = world;
		payload.type_id										  = desc.type_id;
		payload.field_id									  = desc.field_id;
		payload.text_id										  = true;

		if (desc.target.kind == editor_reflected_edit_target_kind_e::world_components)
		{
			SFG_ASSERT(desc.target.entities != nullptr);
			SFG_ASSERT(desc.target.entity_count != 0);
			payload.entities			 = copy_entities_to_aux(command_system, desc.target.entities, desc.target.entity_count);
			payload.entity_count		 = desc.target.entity_count;
			payload.target.entities		 = nullptr;
			payload.target.entity_count	 = desc.target.entity_count;
			chunk_handle32_t* old_values = nullptr;
			payload.old_values			 = allocate_chunk_handle_array(command_system, desc.target.entity_count, old_values);
			world_t& target_world		 = editor_app_t::get().get_runtime().get_world(world);
			for (u32 i = 0; i < desc.target.entity_count; ++i)
			{
				void* object = resolve_world_component_object(target_world, desc.target.type_id, desc.target.entities[i]);
				SFG_ASSERT(object != nullptr);
				u32		   old_text_id = ECS_INVALID_INDEX;
				const bool read		   = read_text_id_field(payload, object, old_text_id);
				SFG_ASSERT(read);
				if (!read)
				{
					SFG_ERR("failed to read reflected text id field {0} for entity {1}", desc.field_id, desc.target.entities[i]);
					return false;
				}
				const char* entity_old_text = target_world.get_text(old_text_id);
				old_values[i]				= copy_text_to_aux(command_system, entity_old_text != nullptr ? entity_old_text : "");
			}
		}

		const editor_command_issue_desc_t issue_desc{
			.undo		= reflected_field_edit_undo,
			.redo		= reflected_field_edit_redo,
			.cleanup	= reflected_field_edit_cleanup,
			.debug_name = "Edit Reflected Text",
			.type		= editor_command_type_e::reflected_field_edit,
			.run_redo	= true,
		};

		const editor_command_handle_t handle = command_system.issue_command(issue_desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue reflected text edit command");
			return false;
		}

		return true;
	}

	const editor_command_reflected_field_edit_payload_t* editor_commands_reflection_t::get_payload(const editor_command_system_t& system, const editor_command_t& command)
	{
		if (command.type != editor_command_type_e::reflected_field_edit)
			return nullptr;
		return &system.get_payload_as<editor_command_reflected_field_edit_payload_t>(command);
	}
}
