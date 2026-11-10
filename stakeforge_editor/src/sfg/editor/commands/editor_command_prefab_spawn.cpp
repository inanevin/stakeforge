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

#include "editor/commands/editor_command_prefab_spawn.hpp"
#include "editor_command_entity_util.hpp"
#include "editor/editor_world_controller.hpp"
#include "editor/world/editor_world.hpp"
#include "editor/editor_command_system.hpp"
#include "editor/world/editor_world_edit_context.hpp"
#include <sfg/core/data/istream.hpp>
#include <sfg/core/data/ostream.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/runtime/resources/world_cook.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/runtime/engine/engine_runtime.hpp>
#include <sfg/core/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		bool prefab_spawn_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			const entity_id_t					   root	   = world.find_by_guid(payload.root_guid);

			if (root == NULL_ENTITY_ID)
				return false;

			world.destroy_entity_tree(root);
			editor_command_entity_util_t::restore_selection(payload.previous_selection_context, system.get_aux_data(), payload.previous_selection, payload.previous_anchor_guid);

			return true;
		}

		bool prefab_spawn_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			world_t&							   world   = editor_world_controller_t::get().get_editor_world(payload.world)->get_world();
			const entity_id_t					   parent  = world.find_by_guid(payload.parent_guid);
			entity_id_t							   root	   = NULL_ENTITY_ID;

			if (payload.parent_guid != NULL_ENTITY_GUID && parent == NULL_ENTITY_ID)
				return false;

			if (payload.snapshot)
			{
				if (!editor_command_entity_util_t::can_restore_tree(world, system.get_aux_data(), payload.snapshot))
					return false;

				istream_t stream(system.get_aux_data().get<u8>(payload.snapshot), payload.snapshot.size);

				root = world_cooker_t::entity_from_stream(world, stream, false);
			}
			else
				root = world_cooker_t::spawn_prefab(world, payload.prefab, {});

			if (root == NULL_ENTITY_ID)
				return false;

			world_cooker_t::make_prefab_chain(world, root, payload.prefab);

			if (payload.snapshot)
			{
				world.scan_for_resources(root);
				world.load_all_used_resources();
			}

			if (parent != NULL_ENTITY_ID)
				world.attach_to(root, parent);

			if (payload.place)
			{
				const mat4x3_t parent_matrix = world.calculate_entity_parent_abs_matrix(root);

				world.set_entity_pos_local(root, world_t::abs_pos_to_local(payload.position, parent_matrix));
			}

			if (!payload.snapshot)
			{
				ostream_t stream = {};

				world_cooker_t::entity_to_stream(world, root, stream);
				payload.snapshot = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
				SFG_MEMCPY(system.get_aux_data().get<u8>(payload.snapshot), stream.get_raw(), stream.get_size());
			}

			payload.root_guid = world.get_entity_guid(root);
			editor_world_controller_t::get().get_editor_world(payload.world)->get_edit_context().apply_entity_selection({.data = &root, .size = 1}, root);

			return true;
		}

		bool prefab_spawn_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);

			if (payload.previous_selection)
				system.get_aux_data().free(payload.previous_selection);

			if (payload.snapshot)
				system.get_aux_data().free(payload.snapshot);

			return true;
		}
	}

	entity_id_t editor_command_prefab_spawn_t::spawn(editor_world_handle_t world, resource_handle_t prefab, entity_id_t parent, const vec3f_t* world_position)
	{
		editor_command_system_t&   command_system	= editor_command_system_t::get();
		editor_world_controller_t& world_controller = editor_world_controller_t::get();
		editor_world_t*			   target_world		= world_controller.get_editor_world(world);

		if (parent != NULL_ENTITY_ID && !target_world->get_edit_context().is_entity_child_insertion_allowed(target_world->get_world(), parent))
			return NULL_ENTITY_ID;

		editor_world_edit_context_t&	selection_controller = world_controller.get_editor_world(world_controller.get_main_world_handle())->get_edit_context();
		const span_t<const entity_id_t> selection			 = selection_controller.get_selected_entities();

		editor_command_prefab_spawn_payload_t payload = {
			.world						= world,
			.previous_selection_context = selection_controller.get_world(),
			.prefab						= prefab,
			.position					= world_position != nullptr ? *world_position : vec3f_t::zero,
			.previous_selection			= editor_command_entity_util_t::capture_entities(world_controller.get_editor_world(selection_controller.get_world())->get_world(), command_system.get_aux_data(), selection),
			.parent_guid				= target_world->get_world().get_entity_guid(parent),
			.root_guid					= NULL_ENTITY_GUID,
			.previous_anchor_guid		= world_controller.get_editor_world(selection_controller.get_world())->get_world().get_entity_guid(selection_controller.get_entity_anchor()),
			.place						= world_position != nullptr,
		};

		const editor_command_issue_desc_t desc{
			.undo			   = prefab_spawn_undo,
			.redo			   = prefab_spawn_redo,
			.cleanup		   = prefab_spawn_cleanup,
			.debug_name		   = "Spawn Prefab",
			.type			   = editor_command_type_e::prefab_spawn,
			.world			   = world,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue prefab spawn command");

			return NULL_ENTITY_ID;
		}

		editor_command_t&					   command		  = command_system.get_command(handle);
		editor_command_prefab_spawn_payload_t& stored_payload = command_system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);

		return target_world->get_world().find_by_guid(stored_payload.root_guid);
	}
}
