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
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		world_t& get_world(world_handle_t handle)
		{
			SFG_ASSERT(!handle.is_null());
			return editor_app_t::get().get_runtime().get_world(handle);
		}

		void copy_name(char* dst, const char* src)
		{
			const char*	 text = src != nullptr ? src : "";
			const size_t len  = std::strlen(text);
			const size_t n	  = len < EDITOR_ENTITY_INFO_NAME_SIZE - 1 ? len : EDITOR_ENTITY_INFO_NAME_SIZE - 1;
			SFG_MEMCPY(dst, text, n);
			dst[n] = '\0';
		}

		chunk_handle32_t copy_entities_to_aux(editor_command_system_t& system, const frame_vector_t<entity_id_t>& entities)
		{
			entity_id_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_id_t>(entities.size(), dst);
			SFG_MEMCPY(dst, entities.data(), sizeof(entity_id_t) * entities.size());
			return handle;
		}

		chunk_handle32_t copy_entity_infos_to_aux(editor_command_system_t& system, world_t& world, const frame_vector_t<entity_id_t>& entities)
		{
			const chunk_handle32_t	   handle = system.get_aux_data().allocate_bytes(sizeof(editor_entity_info_data_t) * entities.size(), alignof(editor_entity_info_data_t));
			editor_entity_info_data_t* dst	  = system.get_aux_data().get<editor_entity_info_data_t>(handle);
			for (size_t i = 0; i < entities.size(); ++i)
				dst[i] = editor_commands_entity_info_t::read(world, entities[i]);
			return handle;
		}

		bool paste_entity_info_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_entity_info_payload_t& payload	  = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			world_t&									world	  = get_world(payload.world);
			const entity_id_t*							entities  = system.get_aux_data().get<entity_id_t>(payload.entities);
			const editor_entity_info_data_t*			old_infos = system.get_aux_data().get<editor_entity_info_data_t>(payload.old_infos);
			for (u32 i = 0; i < payload.count; ++i)
				editor_commands_entity_info_t::apply(world, entities[i], old_infos[i]);
			return true;
		}

		bool paste_entity_info_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_paste_entity_info_payload_t& payload	 = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			world_t&									world	 = get_world(payload.world);
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

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();
		world_t&				 target_world	= get_world(world);

		editor_command_paste_entity_info_payload_t payload = {};
		payload.old_infos								   = copy_entity_infos_to_aux(command_system, target_world, entities);
		payload.entities								   = copy_entities_to_aux(command_system, entities);
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
}
