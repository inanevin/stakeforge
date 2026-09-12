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
#include <sfg/data/span.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/chunk_handle.hpp>

namespace sfg
{
#define EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE 64

	struct editor_command_create_world_folder_payload_t
	{
		char				  name[EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE] = {};
		color_t				  color												= {};
		editor_world_handle_t context											= {};
		u64					  folder_guid										= 0;
		u64					  parent_folder_guid								= 0;
		bool				  folded											= false;
	};

	struct editor_command_rename_world_folder_payload_t
	{
		char				  old_name[EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE] = {};
		char				  new_name[EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE] = {};
		editor_world_handle_t context												= {};
		u64					  folder_guid											= 0;
	};

	struct editor_command_color_world_folder_payload_t
	{
		color_t				  old_color	  = {};
		color_t				  new_color	  = {};
		editor_world_handle_t context	  = {};
		u64					  folder_guid = 0;
	};

	struct editor_command_assign_world_folder_payload_t
	{
		chunk_handle32_t	  entity_guids			= {};
		chunk_handle32_t	  previous_folder_guids = {};
		editor_world_handle_t context				= {};
		u64					  target_folder_guid	= 0;
		u32					  count					= 0;
	};

	struct editor_command_assign_world_folder_parent_payload_t
	{
		editor_world_handle_t context			   = {};
		u64					  folder_guid		   = 0;
		u64					  previous_parent_guid = 0;
		u64					  next_parent_guid	   = 0;
	};

	class editor_commands_world_edit_context_t final
	{
	public:
		editor_commands_world_edit_context_t() = delete;

		static editor_world_folder_handle_t create_folder(const char* name, editor_world_folder_handle_t parent_handle = {});
		static editor_world_folder_handle_t create_folder(editor_world_handle_t context, const char* name, editor_world_folder_handle_t parent_handle = {});
		static bool							rename_folder(editor_world_folder_handle_t handle, const char* name);
		static bool							rename_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, const char* name);
		static bool							change_folder_color(editor_world_folder_handle_t handle, color_t color);
		static bool							change_folder_color(editor_world_handle_t context, editor_world_folder_handle_t handle, color_t color);
		static bool							delete_folder(editor_world_folder_handle_t handle);
		static bool							delete_folder(editor_world_handle_t context, editor_world_folder_handle_t handle);
		static bool							assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids);
		static bool							assign_entities_to_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids);
		static bool							deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids);
		static bool							deassign_entities_from_folder(editor_world_handle_t context, span_t<const entity_guid_t> entity_guids);
		static bool							assign_folder_to_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle);
		static bool							assign_folder_to_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle);
	};
}
