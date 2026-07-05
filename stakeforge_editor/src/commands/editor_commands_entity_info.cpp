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
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "commands/editor_commands_entity_info.hpp"
#include "editor_world_controller.hpp"
#include "editor_command_system.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		void copy_name(char* dst, const char* src)
		{
			const char*	 text = src != nullptr ? src : "";
			const size_t len  = std::strlen(text);
			const size_t n	  = len < EDITOR_ENTITY_INFO_NAME_SIZE - 1 ? len : EDITOR_ENTITY_INFO_NAME_SIZE - 1;
			SFG_MEMCPY(dst, text, n);
			dst[n] = '\0';
		}

		chunk_handle32_t copy_entities_to_aux(editor_command_system_t& system, span_t<const entity_id_t> entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size, dst);
			SFG_MEMCPY(dst, entities.data, sizeof(entity_id_t) * entities.size);
			return handle;
		}

		chunk_handle32_t copy_entity_infos_to_aux(editor_command_system_t& system, world_t& world, span_t<const entity_id_t> entities)
		{
			const chunk_handle32_t	   handle = system.get_aux_data().allocate_bytes(sizeof(editor_entity_info_data_t) * entities.size, alignof(editor_entity_info_data_t));
			editor_entity_info_data_t* dst	  = system.get_aux_data().get<editor_entity_info_data_t>(handle);
			for (size_t i = 0; i < entities.size; ++i)
				dst[i] = editor_commands_entity_info_t::read(world, entities.data[i]);
			return handle;
		}

		chunk_handle32_t copy_entity_infos_to_aux(editor_command_system_t& system, span_t<const editor_entity_info_data_t> infos)
		{
			const chunk_handle32_t	   handle = system.get_aux_data().allocate_bytes(sizeof(editor_entity_info_data_t) * infos.size, alignof(editor_entity_info_data_t));
			editor_entity_info_data_t* dst	  = system.get_aux_data().get<editor_entity_info_data_t>(handle);
			SFG_MEMCPY(dst, infos.data, sizeof(editor_entity_info_data_t) * infos.size);
			return handle;
		}

		bool entity_infos_equal(span_t<const editor_entity_info_data_t> a, span_t<const editor_entity_info_data_t> b)
		{
			if (a.size != b.size)
				return false;

			for (size_t i = 0; i < a.size; ++i)
			{
				if (std::strncmp(a.data[i].name, b.data[i].name, EDITOR_ENTITY_INFO_NAME_SIZE) != 0)
					return false;
				if (a.data[i].pos != b.data[i].pos)
					return false;
				if (a.data[i].rot != b.data[i].rot)
					return false;
				if (a.data[i].scale != b.data[i].scale)
					return false;
			}
			return true;
		}

		void free_edit_entity_info_payload(editor_command_system_t& system, editor_command_edit_entity_info_payload_t& payload)
		{
			if (payload.previous_infos)
			{
				system.get_aux_data().free(payload.previous_infos);
				payload.previous_infos = {};
			}
			if (payload.post_infos)
			{
				system.get_aux_data().free(payload.post_infos);
				payload.post_infos = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
		}

		void apply_entity_infos(editor_command_system_t& system, editor_command_edit_entity_info_payload_t& payload, chunk_handle32_t infos_handle)
		{
			world_t&						 world	  = editor_world_controller_t::get().get_world(payload.world);
			const entity_id_t*				 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const editor_entity_info_data_t* infos	  = system.get_aux_data().get<editor_entity_info_data_t>(infos_handle);
			for (u32 i = 0; i < payload.count; ++i)
				editor_commands_entity_info_t::apply(world, entities[i], infos[i]);
		}

		bool paste_entity_info_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_entity_info_payload_t& payload	  = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			world_t&									world	  = editor_world_controller_t::get().get_world(payload.world);
			const entity_id_t*							entities  = system.get_aux_data().get<entity_id_t>(payload.entities);
			const editor_entity_info_data_t*			old_infos = system.get_aux_data().get<editor_entity_info_data_t>(payload.old_infos);
			for (u32 i = 0; i < payload.count; ++i)
				editor_commands_entity_info_t::apply(world, entities[i], old_infos[i]);
			return true;
		}

		bool paste_entity_info_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_entity_info_payload_t& payload	 = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			world_t&									world	 = editor_world_controller_t::get().get_world(payload.world);
			const entity_id_t*							entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
				editor_commands_entity_info_t::apply(world, entities[i], payload.info);
			return true;
		}

		bool paste_entity_info_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_entity_info_payload_t& payload = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			if (payload.old_infos)
			{
				system.get_aux_data().free(payload.old_infos);
				payload.old_infos = {};
			}
			if (payload.entities)
			{
				system.get_aux_data().free(payload.entities);
				payload.entities = {};
			}
			return true;
		}

		bool edit_entity_info_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_edit_entity_info_payload_t& payload = system.get_payload_as<editor_command_edit_entity_info_payload_t>(command);
			apply_entity_infos(system, payload, payload.previous_infos);
			return true;
		}

		bool edit_entity_info_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_edit_entity_info_payload_t& payload = system.get_payload_as<editor_command_edit_entity_info_payload_t>(command);
			apply_entity_infos(system, payload, payload.post_infos);
			return true;
		}

		bool edit_entity_info_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_edit_entity_info_payload_t& payload = system.get_payload_as<editor_command_edit_entity_info_payload_t>(command);
			free_edit_entity_info_payload(system, payload);
			return true;
		}
	}

	editor_entity_info_data_t editor_commands_entity_info_t::read(world_t& world, entity_id_t entity)
	{
		editor_entity_info_data_t info = {};
		copy_name(info.name, world.get_entity_name(entity));
		info.pos   = world.get_entity_pos_local(entity);
		info.rot   = world.get_entity_rot_local(entity);
		info.scale = world.get_entity_scale_local(entity);
		return info;
	}

	void editor_commands_entity_info_t::apply(world_t& world, entity_id_t entity, const editor_entity_info_data_t& info)
	{
		world.set_entity_name(entity, info.name);
		world.set_entity_pos_local(entity, info.pos);
		world.set_entity_rot_local(entity, info.rot);
		world.set_entity_scale_local(entity, info.scale);
	}

	bool editor_commands_entity_info_t::paste(world_handle_t world, entity_id_t entity, const editor_entity_info_data_t& info)
	{
		frame_vector_t<entity_id_t> entities;
		entities.push_back(entity);
		return paste(world, entities, info);
	}

	bool editor_commands_entity_info_t::paste(world_handle_t world, const frame_vector_t<entity_id_t>& entities, const editor_entity_info_data_t& info)
	{
		if (entities.empty())
			return false;
		SFG_ASSERT(entities.size() <= UINT32_MAX);

		editor_command_system_t& command_system = editor_command_system_t::get();
		world_t&				 target_world	= editor_world_controller_t::get().get_world(world);

		editor_command_paste_entity_info_payload_t payload = {};
		payload.old_infos								   = copy_entity_infos_to_aux(command_system, target_world, {.data = entities.data(), .size = entities.size()});
		payload.entities								   = copy_entities_to_aux(command_system, {.data = entities.data(), .size = entities.size()});
		payload.info									   = info;
		payload.world									   = world;
		payload.count									   = static_cast<u32>(entities.size());

		const editor_command_issue_desc_t desc{
			.undo		= paste_entity_info_undo,
			.redo		= paste_entity_info_redo,
			.cleanup	= paste_entity_info_cleanup,
			.debug_name = "Paste Entity Info",
			.type		= editor_command_type_e::entity_info_paste,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue paste entity info command");
			return false;
		}

		return true;
	}

	bool editor_commands_entity_info_t::edit(world_handle_t world, span_t<const entity_id_t> entities, span_t<const editor_entity_info_data_t> previous_infos, span_t<const editor_entity_info_data_t> post_infos)
	{
		if (entities.size == 0 || previous_infos.size != entities.size || post_infos.size != entities.size)
			return false;
		if (entity_infos_equal(previous_infos, post_infos))
			return true;
		SFG_ASSERT(entities.size <= UINT32_MAX);

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_edit_entity_info_payload_t payload = {};
		payload.previous_infos							  = copy_entity_infos_to_aux(command_system, previous_infos);
		payload.post_infos								  = copy_entity_infos_to_aux(command_system, post_infos);
		payload.entities								  = copy_entities_to_aux(command_system, entities);
		payload.world									  = world;
		payload.count									  = static_cast<u32>(entities.size);

		const editor_command_issue_desc_t desc{
			.undo		= edit_entity_info_undo,
			.redo		= edit_entity_info_redo,
			.cleanup	= edit_entity_info_cleanup,
			.debug_name = "Entity Info Edit",
			.type		= editor_command_type_e::entity_info_edit,
			.run_redo	= false,
			.notify		= false,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue entity info edit command");
			free_edit_entity_info_payload(command_system, payload);
			return false;
		}

		return true;
	}
}
