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

#include "commands/editor_command_primitive_spawn.hpp"
#include "assets/editor_asset.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_edit_context.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		const char* get_primitive_name(editor_primitive_type_e primitive)
		{
			switch (primitive)
			{
			case editor_primitive_type_e::cube:
				return "Cube";
			case editor_primitive_type_e::sphere:
				return "Sphere";
			case editor_primitive_type_e::cylinder:
				return "Cylinder";
			case editor_primitive_type_e::capsule:
				return "Capsule";
			default:
				SFG_ASSERT(false);
				return "Primitive";
			}
		}

		resource_handle_t get_primitive_mesh(editor_primitive_type_e primitive)
		{
			switch (primitive)
			{
			case editor_primitive_type_e::cube:
				return DEFAULT_MESH_CUBE_GUID;
			case editor_primitive_type_e::sphere:
				return DEFAULT_MESH_SPHERE_GUID;
			case editor_primitive_type_e::cylinder:
				return DEFAULT_MESH_CYLINDER_GUID;
			case editor_primitive_type_e::capsule:
				return DEFAULT_MESH_CAPSULE_GUID;
			default:
				SFG_ASSERT(false);
				return DEFAULT_MESH_CUBE_GUID;
			}
		}

		bool primitive_spawn_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_primitive_spawn_payload_t& payload = system.get_payload_as<editor_command_primitive_spawn_payload_t>(command);
			world_t&								  world	  = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			if (payload.folder_guid != 0 && payload.guid != NULL_ENTITY_GUID)
			{
				const entity_guid_t guid = payload.guid;
				editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context().deassign_entities_from_folder({.data = &guid, .size = 1});
			}
			world.destroy_entity_tree(payload.entity);
			payload.entity						  = NULL_ENTITY_ID;
			const entity_id_t* previous_selection = payload.previous_selection_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.previous_selection) : nullptr;
			editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context().apply_entity_selection({.data = previous_selection, .size = payload.previous_selection_count}, payload.previous_anchor);
			return true;
		}

		bool primitive_spawn_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_primitive_spawn_payload_t& payload = system.get_payload_as<editor_command_primitive_spawn_payload_t>(command);
			world_t&								  world	  = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();

			const entity_id_t entity = world.create_entity(get_primitive_name(payload.primitive), payload.guid);
			if (payload.parent != NULL_ENTITY_ID)
				world.attach_to(entity, payload.parent);

			component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(world.get_component_table(type_id_t<component_mesh_renderer_t>::value), entity);
			mesh_renderer.mesh						 = get_primitive_mesh(payload.primitive);
			mesh_renderer.materials.resize(0);
			mesh_renderer.materials.push_back(DEFAULT_GBUFFER_MATERIAL_ASSET_GUID);
			world.scan_for_resources(entity, true);

			payload.guid   = world.get_entity_guid(entity);
			payload.entity = entity;
			if (payload.folder_guid != 0)
			{
				editor_world_edit_context_t&	   metadata = editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context();
				const editor_world_folder_handle_t folder	= metadata.get_folder_handle(payload.folder_guid);
				if (!folder.is_null())
					metadata.assign_entities_to_folder(folder, {.data = &payload.guid, .size = 1});
			}
			editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context().apply_entity_selection({.data = &payload.entity, .size = 1}, payload.entity);
			return true;
		}

		bool primitive_spawn_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_primitive_spawn_payload_t& payload = system.get_payload_as<editor_command_primitive_spawn_payload_t>(command);
			if (payload.previous_selection)
			{
				system.get_aux_data().free(payload.previous_selection);
				payload.previous_selection = {};
			}
			return true;
		}
	}

	entity_id_t editor_command_primitive_spawn_t::spawn(editor_world_handle_t world, editor_primitive_type_e primitive, entity_id_t parent, editor_world_folder_handle_t folder)
	{
		editor_command_system_t&		command_system = editor_command_system_t::get();
		editor_world_edit_context_t&	context		   = editor_world_controller_t::get().get_editor_world(world)->get_edit_context();
		const span_t<const entity_id_t> selection	   = context.get_selected_entities();
		SFG_ASSERT(selection.size <= UINT32_MAX);

		editor_command_primitive_spawn_payload_t payload = {
			.previous_selection		  = {},
			.world					  = world,
			.parent					  = parent,
			.entity					  = NULL_ENTITY_ID,
			.previous_anchor		  = context.get_entity_anchor(),
			.guid					  = NULL_ENTITY_GUID,
			.folder_guid			  = folder.is_null() ? 0 : context.get_folder(folder).guid,
			.previous_selection_count = static_cast<u32>(selection.size),
			.primitive				  = primitive,
		};

		if (selection.size != 0)
		{
			entity_id_t* dst		   = nullptr;
			payload.previous_selection = command_system.get_aux_data().allocate<entity_id_t>(selection.size, dst);
			SFG_MEMCPY(dst, selection.data, sizeof(entity_id_t) * selection.size);
		}

		const editor_command_issue_desc_t desc{
			.undo			   = primitive_spawn_undo,
			.redo			   = primitive_spawn_redo,
			.cleanup		   = primitive_spawn_cleanup,
			.debug_name		   = "Spawn Primitive",
			.type			   = editor_command_type_e::primitive_spawn,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			if (payload.previous_selection)
				command_system.get_aux_data().free(payload.previous_selection);
			SFG_ERR("failed to issue primitive spawn command");
			return NULL_ENTITY_ID;
		}

		editor_command_t&						  command		 = command_system.get_command(handle);
		editor_command_primitive_spawn_payload_t& stored_payload = command_system.get_payload_as<editor_command_primitive_spawn_payload_t>(command);
		return stored_payload.entity;
	}
}
