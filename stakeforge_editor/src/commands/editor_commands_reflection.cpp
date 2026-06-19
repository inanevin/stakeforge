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
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
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

		void* resolve_target_object(editor_command_system_t& system, const editor_command_reflected_field_edit_payload_t& payload)
		{
			switch (payload.target.kind)
			{
			case editor_reflected_edit_target_kind_e::raw_object:
				if (!payload.target.required_listener.is_null() && !system.is_listener_valid(payload.target.required_listener))
					return nullptr;
				return payload.target.object;
			case editor_reflected_edit_target_kind_e::world_component: {
				world_t&				 world = editor_app_t::get().get_runtime().get_world(payload.target.world);
				world_component_table_t* table = world.find_component_table(payload.target.type_id);
				if (table == nullptr || !ecs_t::table_has(table->table, payload.target.entity))
					return nullptr;
				return ecs_t::table_get(table->table, payload.target.entity);
			}
			default:
				return nullptr;
			}
		}

		bool apply_value(editor_command_system_t& system, editor_command_reflected_field_edit_payload_t& payload, chunk_handle32_t value)
		{
			void* object = resolve_target_object(system, payload);
			if (object == nullptr)
				return true;

			istream_t stream(system.get_aux_data().get<u8>(value), value.size);
			return reflection_registry_t::get().deserialize_field_from_stream(payload.type_id, payload.field_id, object, stream);
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
			return true;
		}
	}

	bool editor_commands_reflection_t::edit_field(const editor_reflected_field_edit_desc_t& desc, const ostream_t& old_value, const ostream_t& new_value)
	{
		if (old_value.get_size() == new_value.get_size() && (old_value.get_size() == 0 || std::memcmp(old_value.get_raw(), new_value.get_raw(), old_value.get_size()) == 0))
			return true;

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_reflected_field_edit_payload_t payload = {};
		payload.old_value									  = copy_stream_to_aux(command_system, old_value);
		payload.new_value									  = copy_stream_to_aux(command_system, new_value);
		payload.target										  = desc.target;
		payload.type_id										  = desc.type_id;
		payload.field_id									  = desc.field_id;

		const editor_command_issue_desc_t issue_desc{
			.undo		= reflected_field_edit_undo,
			.redo		= reflected_field_edit_redo,
			.cleanup	= reflected_field_edit_cleanup,
			.debug_name = "Edit Reflected Field",
			.type		= editor_command_type_e::reflected_field_edit,
			.run_redo	= false,
		};

		return !command_system.issue_command(issue_desc, payload).is_null();
	}

	const editor_command_reflected_field_edit_payload_t* editor_commands_reflection_t::get_payload(const editor_command_system_t& system, const editor_command_t& command)
	{
		if (command.type != editor_command_type_e::reflected_field_edit)
			return nullptr;
		return &system.get_payload_as<editor_command_reflected_field_edit_payload_t>(command);
	}
}
