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
