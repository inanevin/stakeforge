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
#include <sfg/data/span.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

#define EDITOR_ENTITY_INFO_NAME_SIZE 256

	struct editor_entity_info_data_t
	{
		char	name[EDITOR_ENTITY_INFO_NAME_SIZE] = {};
		vec3f_t pos								   = vec3f_t::zero;
		quat_t	rot								   = {};
		vec3f_t scale							   = vec3f_t::one;
	};

	struct editor_command_paste_entity_info_payload_t
	{
		chunk_handle32_t		  old_infos = {};
		chunk_handle32_t		  entities	= {};
		editor_entity_info_data_t info		= {};
		editor_world_handle_t	  world		= {};
		u32						  count		= 0;
	};

	struct editor_command_edit_entity_info_payload_t
	{
		chunk_handle32_t	  previous_infos = {};
		chunk_handle32_t	  post_infos	 = {};
		chunk_handle32_t	  entities		 = {};
		editor_world_handle_t world			 = {};
		u32					  count			 = 0;
	};

	class editor_commands_entity_info_t final
	{
	public:
		editor_commands_entity_info_t() = delete;

		static editor_entity_info_data_t read(world_t& world, entity_id_t entity);
		static void						 apply(world_t& world, entity_id_t entity, const editor_entity_info_data_t& info);
		static bool						 paste(editor_world_handle_t world, entity_id_t entity, const editor_entity_info_data_t& info);
		static bool						 paste(editor_world_handle_t world, const frame_vector_t<entity_id_t>& entities, const editor_entity_info_data_t& info);
		static bool						 edit(editor_world_handle_t world, span_t<const entity_id_t> entities, span_t<const editor_entity_info_data_t> previous_infos, span_t<const editor_entity_info_data_t> post_infos);
	};
}
