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
#include <sfg/core/math/vec3f.hpp>
#include <sfg/core/runtime/resources/resource_handle.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>
#include <sfg/core/memory/chunk_handle.hpp>

namespace sfg
{
	struct editor_command_prefab_spawn_payload_t
	{
		editor_world_handle_t world						 = {};
		editor_world_handle_t previous_selection_context = {};
		resource_handle_t	  prefab					 = NULL_RESOURCE_HANDLE;
		vec3f_t				  position					 = vec3f_t::zero;
		chunk_handle32_t	  previous_selection		 = {};
		chunk_handle32_t	  snapshot					 = {};
		entity_guid_t		  parent_guid				 = NULL_ENTITY_GUID;
		entity_guid_t		  root_guid					 = NULL_ENTITY_GUID;
		entity_guid_t		  previous_anchor_guid		 = NULL_ENTITY_GUID;
		bool				  place						 = false;
	};

	class SFG_EDITOR_API editor_command_prefab_spawn_t final
	{
	public:
		editor_command_prefab_spawn_t() = delete;

		static entity_id_t spawn(editor_world_handle_t world, resource_handle_t prefab, entity_id_t parent, const vec3f_t* world_position = nullptr);
	};
}
