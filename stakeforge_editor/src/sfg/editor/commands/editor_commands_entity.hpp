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

#pragma once

#include "editor/editor_exports.hpp"

#include "editor/world/editor_world_edit_context.hpp"
#include "editor/world/editor_world_handle.hpp"
#include <sfg/core/data/frame_vector.hpp>
#include <sfg/core/memory/chunk_handle.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>

namespace sfg
{
#define EDITOR_ENTITY_COMMAND_NAME_SIZE 64

	enum class editor_entity_preset_e : u8
	{
		empty,
		camera,
		directional_light,
		point_light,
		spot_light,
		area_light,
		ground,
		character,
	};

	struct editor_command_create_entity_payload_t
	{
		chunk_handle32_t	   previous_selection					 = {};
		editor_world_handle_t  world								 = {};
		entity_guid_t		   parent_guid							 = NULL_ENTITY_GUID;
		entity_guid_t		   previous_anchor_guid					 = NULL_ENTITY_GUID;
		entity_guid_t		   guid									 = NULL_ENTITY_GUID;
		u64					   folder_guid							 = 0;
		char				   name[EDITOR_ENTITY_COMMAND_NAME_SIZE] = {};
		editor_entity_preset_e preset								 = editor_entity_preset_e::empty;
	};

	struct editor_command_duplicate_entity_payload_t
	{
		chunk_handle32_t	  previous_selection   = {};
		chunk_handle32_t	  streams			   = {};
		chunk_handle32_t	  source_guids		   = {};
		chunk_handle32_t	  parent_guids		   = {};
		chunk_handle32_t	  entity_guids		   = {};
		editor_world_handle_t world				   = {};
		entity_guid_t		  previous_anchor_guid = NULL_ENTITY_GUID;
		u32					  count				   = 0;
	};

	struct editor_command_destroy_entity_payload_t
	{
		chunk_handle32_t	  previous_selection   = {};
		chunk_handle32_t	  streams			   = {};
		chunk_handle32_t	  entity_guids		   = {};
		editor_world_handle_t world				   = {};
		entity_guid_t		  previous_anchor_guid = NULL_ENTITY_GUID;
		u32					  count				   = 0;
	};

	struct editor_command_reparent_entity_payload_t
	{
		chunk_handle32_t	  entity_guids			= {};
		chunk_handle32_t	  previous_parent_guids = {};
		chunk_handle32_t	  next_parent_guids		= {};
		editor_world_handle_t world					= {};
		u32					  count					= 0;
	};

	class SFG_EDITOR_API editor_commands_entity_t final
	{
	public:
		editor_commands_entity_t() = delete;

		static entity_id_t create(editor_world_handle_t world, entity_id_t parent, editor_world_folder_handle_t folder = {}, editor_entity_preset_e preset = editor_entity_preset_e::empty);
		static entity_id_t duplicate(editor_world_handle_t world, entity_id_t entity);
		static bool		   duplicate(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, frame_vector_t<entity_id_t>& out_entities);
		static bool		   destroy(editor_world_handle_t world, entity_id_t entity);
		static bool		   destroy(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities);
		static bool		   reparent(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, entity_id_t parent);
	};
}
