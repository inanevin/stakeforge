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

#include "world/editor_world_edit_context.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	enum class editor_primitive_type_e : u8
	{
		cube,
		sphere,
		cylinder,
		capsule,
		plane,
	};

	struct editor_command_primitive_spawn_payload_t
	{
		chunk_handle32_t		previous_selection		 = {};
		editor_world_handle_t	world					 = {};
		entity_id_t				parent					 = NULL_ENTITY_ID;
		entity_id_t				entity					 = NULL_ENTITY_ID;
		entity_id_t				previous_anchor			 = NULL_ENTITY_ID;
		entity_guid_t			guid					 = NULL_ENTITY_GUID;
		u64						folder_guid				 = 0;
		u32						previous_selection_count = 0;
		editor_primitive_type_e primitive				 = editor_primitive_type_e::cube;
	};

	class editor_command_primitive_spawn_t final
	{
	public:
		editor_command_primitive_spawn_t() = delete;

		static entity_id_t spawn(editor_world_handle_t world, editor_primitive_type_e primitive, entity_id_t parent, editor_world_folder_handle_t folder = {});
	};
}
