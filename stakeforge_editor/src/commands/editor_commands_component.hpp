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

#include "world/editor_world_handle.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct editor_command_add_component_payload_t
	{
		chunk_handle32_t	  entities		 = {};
		editor_world_handle_t world			 = {};
		sid_t				  component_type = 0;
		u32					  count			 = 0;
	};

	struct editor_command_remove_component_payload_t
	{
		chunk_handle32_t	  streams		 = {};
		chunk_handle32_t	  entities		 = {};
		editor_world_handle_t world			 = {};
		sid_t				  component_type = 0;
		u32					  count			 = 0;
	};

	struct editor_command_reset_component_payload_t
	{
		chunk_handle32_t	  streams		 = {};
		chunk_handle32_t	  entities		 = {};
		editor_world_handle_t world			 = {};
		sid_t				  component_type = 0;
		u32					  count			 = 0;
	};

	struct editor_command_paste_component_payload_t
	{
		chunk_handle32_t	  old_streams	 = {};
		chunk_handle32_t	  paste_stream	 = {};
		chunk_handle32_t	  entities		 = {};
		editor_world_handle_t world			 = {};
		sid_t				  component_type = 0;
		u32					  count			 = 0;
	};

	class editor_commands_component_t final
	{
	public:
		editor_commands_component_t() = delete;

		static bool add(editor_world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool add(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool remove(editor_world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool remove(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool reset(editor_world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool reset(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool paste(editor_world_handle_t world, entity_id_t entity, sid_t component_type, const u8* data, size_t data_size);
		static bool paste(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type, const u8* data, size_t data_size);
	};
}
