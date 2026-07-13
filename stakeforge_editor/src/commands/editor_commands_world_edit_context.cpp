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
#include "commands/editor_commands_world_edit_context.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <cstring>

namespace sfg
{
	namespace
	{
		struct deleted_world_folder_t
		{
			char name[EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE];
			f32	 color[4];
			u64	 folder_guid;
			u64	 parent_folder_guid;
			u32	 first_entity_guid;
			u32	 entity_guid_count;
			bool folded;
		};

		struct editor_command_delete_world_folder_payload_t
		{
			chunk_handle32_t	  folders			= {};
			chunk_handle32_t	  entity_guids		= {};
			editor_world_handle_t context			= {};
			u64					  root_folder_guid	= 0;
			u32					  folder_count		= 0;
			u32					  entity_guid_count = 0;
		};

		void copy_name(char* dst, const char* src)
		{
			const char*	 text = src != nullptr && src[0] != '\0' ? src : "Folder";
			const size_t len  = std::strlen(text);
			const size_t n	  = len < EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE - 1 ? len : EDITOR_WORLD_EDIT_CONTEXT_COMMAND_NAME_SIZE - 1;
			SFG_MEMCPY(dst, text, n);
			dst[n] = '\0';
		}

		u64 get_folder_guid(editor_world_edit_context_t& metadata, editor_world_folder_handle_t handle)
		{
			return handle.is_null() ? 0 : metadata.get_folder(handle).guid;
		}

		editor_world_folder_handle_t get_folder_handle(editor_world_edit_context_t& metadata, u64 guid)
		{
			return guid == 0 ? editor_world_folder_handle_t{} : metadata.get_folder_handle(guid);
		}

		chunk_handle32_t copy_entity_guids_to_aux(editor_command_system_t& system, span_t<const entity_guid_t> entity_guids)
		{
			entity_guid_t*		   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<entity_guid_t>(entity_guids.size, dst);
			SFG_MEMCPY(dst, entity_guids.data, sizeof(entity_guid_t) * entity_guids.size);
			return handle;
		}

		chunk_handle32_t copy_previous_folder_guids_to_aux(editor_command_system_t& system, editor_world_edit_context_t& metadata, span_t<const entity_guid_t> entity_guids)
		{
			u64*				   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<u64>(entity_guids.size, dst);
			for (size_t i = 0; i < entity_guids.size; ++i)
				dst[i] = get_folder_guid(metadata, metadata.get_entity_folder(entity_guids.data[i]));
			return handle;
		}

		void destroy_folder_tree(editor_world_edit_context_t& metadata, editor_world_folder_handle_t handle)
		{
			vector_t<editor_world_folder_handle_t> folders;
			metadata.collect_folder_tree(handle, folders);
			for (auto it = folders.rbegin(); it != folders.rend(); ++it)
				metadata.destroy_folder(*it);
		}

		bool create_world_folder_undo(editor_command_system_t&, editor_command_t& command)
		{
			editor_command_create_world_folder_payload_t& payload  = editor_command_system_t::get().get_payload_as<editor_command_create_world_folder_payload_t>(command);
			editor_world_edit_context_t&				  metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t			  handle   = get_folder_handle(metadata, payload.folder_guid);
			if (!handle.is_null())
				metadata.destroy_folder(handle);
			return true;
		}

		bool create_world_folder_redo(editor_command_system_t&, editor_command_t& command)
		{
			editor_command_create_world_folder_payload_t& payload		= editor_command_system_t::get().get_payload_as<editor_command_create_world_folder_payload_t>(command);
			editor_world_edit_context_t&				  metadata		= editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t			  parent_handle = get_folder_handle(metadata, payload.parent_folder_guid);
			const editor_world_folder_handle_t			  handle		= metadata.create_folder_with_guid(payload.name, payload.color, payload.folded, parent_handle, payload.folder_guid);
			payload.folder_guid											= metadata.get_folder(handle).guid;
			return true;
		}

		bool rename_world_folder_undo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_rename_world_folder_payload_t& payload	 = editor_command_system_t::get().get_payload_as<editor_command_rename_world_folder_payload_t>(command);
			editor_world_edit_context_t&						metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t					handle	 = get_folder_handle(metadata, payload.folder_guid);
			if (!handle.is_null())
				metadata.set_folder_name(handle, payload.old_name);
			return true;
		}

		bool rename_world_folder_redo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_rename_world_folder_payload_t& payload	 = editor_command_system_t::get().get_payload_as<editor_command_rename_world_folder_payload_t>(command);
			editor_world_edit_context_t&						metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t					handle	 = get_folder_handle(metadata, payload.folder_guid);
			if (!handle.is_null())
				metadata.set_folder_name(handle, payload.new_name);
			return true;
		}

		bool color_world_folder_undo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_color_world_folder_payload_t& payload	= editor_command_system_t::get().get_payload_as<editor_command_color_world_folder_payload_t>(command);
			editor_world_edit_context_t&					   metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t				   handle	= get_folder_handle(metadata, payload.folder_guid);
			if (!handle.is_null())
				metadata.set_folder_color(handle, payload.old_color);
			return true;
		}

		bool color_world_folder_redo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_color_world_folder_payload_t& payload	= editor_command_system_t::get().get_payload_as<editor_command_color_world_folder_payload_t>(command);
			editor_world_edit_context_t&					   metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t				   handle	= get_folder_handle(metadata, payload.folder_guid);
			if (!handle.is_null())
				metadata.set_folder_color(handle, payload.new_color);
			return true;
		}

		bool delete_world_folder_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_delete_world_folder_payload_t& payload		 = system.get_payload_as<editor_command_delete_world_folder_payload_t>(command);
			editor_world_edit_context_t&						metadata	 = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const deleted_world_folder_t*						folders		 = system.get_aux_data().get<deleted_world_folder_t>(payload.folders);
			const entity_guid_t*								entity_guids = payload.entity_guid_count != 0 ? system.get_aux_data().get<entity_guid_t>(payload.entity_guids) : nullptr;

			for (u32 i = 0; i < payload.folder_count; ++i)
			{
				const deleted_world_folder_t&	   folder = folders[i];
				const editor_world_folder_handle_t parent = get_folder_handle(metadata, folder.parent_folder_guid);
				metadata.create_folder_with_guid(folder.name, color_t(folder.color[0], folder.color[1], folder.color[2], folder.color[3]), folder.folded, parent, folder.folder_guid);
			}

			for (u32 i = 0; i < payload.folder_count; ++i)
			{
				const deleted_world_folder_t& folder = folders[i];
				if (folder.entity_guid_count == 0)
					continue;

				const editor_world_folder_handle_t handle = get_folder_handle(metadata, folder.folder_guid);
				metadata.assign_entities_to_folder(handle, {.data = entity_guids + folder.first_entity_guid, .size = folder.entity_guid_count});
			}
			return true;
		}

		bool delete_world_folder_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_delete_world_folder_payload_t& payload	 = system.get_payload_as<editor_command_delete_world_folder_payload_t>(command);
			editor_world_edit_context_t&						metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t					handle	 = get_folder_handle(metadata, payload.root_folder_guid);
			if (!handle.is_null())
				destroy_folder_tree(metadata, handle);
			return true;
		}

		bool delete_world_folder_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_delete_world_folder_payload_t& payload = system.get_payload_as<editor_command_delete_world_folder_payload_t>(command);
			if (payload.folders)
			{
				system.get_aux_data().free(payload.folders);
				payload.folders = {};
			}
			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
			}
			return true;
		}

		bool apply_folder_assignments(editor_command_system_t& system, const editor_command_assign_world_folder_payload_t& payload, chunk_handle32_t folder_guids_handle)
		{
			editor_world_edit_context_t& metadata	  = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const entity_guid_t*		 entity_guids = system.get_aux_data().get<entity_guid_t>(payload.entity_guids);
			const u64*					 folder_guids = system.get_aux_data().get<u64>(folder_guids_handle);

			for (u32 i = 0; i < payload.count; ++i)
			{
				const editor_world_folder_handle_t handle = get_folder_handle(metadata, folder_guids[i]);
				const entity_guid_t				   guid	  = entity_guids[i];
				if (handle.is_null())
					metadata.deassign_entities_from_folder({.data = &guid, .size = 1});
				else
					metadata.assign_entities_to_folder(handle, {.data = &guid, .size = 1});
			}
			return true;
		}

		bool assign_world_folder_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_assign_world_folder_payload_t& payload = system.get_payload_as<editor_command_assign_world_folder_payload_t>(command);
			return apply_folder_assignments(system, payload, payload.previous_folder_guids);
		}

		bool assign_world_folder_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_assign_world_folder_payload_t& payload		 = system.get_payload_as<editor_command_assign_world_folder_payload_t>(command);
			editor_world_edit_context_t&						metadata	 = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const entity_guid_t*								entity_guids = system.get_aux_data().get<entity_guid_t>(payload.entity_guids);
			const editor_world_folder_handle_t					handle		 = get_folder_handle(metadata, payload.target_folder_guid);
			if (handle.is_null())
				metadata.deassign_entities_from_folder({.data = entity_guids, .size = payload.count});
			else
				metadata.assign_entities_to_folder(handle, {.data = entity_guids, .size = payload.count});
			return true;
		}

		bool assign_world_folder_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_assign_world_folder_payload_t& payload = system.get_payload_as<editor_command_assign_world_folder_payload_t>(command);
			if (payload.entity_guids)
			{
				system.get_aux_data().free(payload.entity_guids);
				payload.entity_guids = {};
			}
			if (payload.previous_folder_guids)
			{
				system.get_aux_data().free(payload.previous_folder_guids);
				payload.previous_folder_guids = {};
			}
			return true;
		}

		bool assign_world_folder_parent_undo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_assign_world_folder_parent_payload_t& payload	= editor_command_system_t::get().get_payload_as<editor_command_assign_world_folder_parent_payload_t>(command);
			editor_world_edit_context_t&							   metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t						   handle	= get_folder_handle(metadata, payload.folder_guid);
			const editor_world_folder_handle_t						   parent	= get_folder_handle(metadata, payload.previous_parent_guid);
			if (!handle.is_null() && metadata.can_assign_folder(handle, parent))
				metadata.set_folder_parent(handle, parent);
			return true;
		}

		bool assign_world_folder_parent_redo(editor_command_system_t&, editor_command_t& command)
		{
			const editor_command_assign_world_folder_parent_payload_t& payload	= editor_command_system_t::get().get_payload_as<editor_command_assign_world_folder_parent_payload_t>(command);
			editor_world_edit_context_t&							   metadata = editor_world_controller_t::get().get_editor_world(payload.context)->get_edit_context();
			const editor_world_folder_handle_t						   handle	= get_folder_handle(metadata, payload.folder_guid);
			const editor_world_folder_handle_t						   parent	= get_folder_handle(metadata, payload.next_parent_guid);
			if (!handle.is_null() && metadata.can_assign_folder(handle, parent))
				metadata.set_folder_parent(handle, parent);
			return true;
		}
	}

	editor_world_folder_handle_t editor_commands_world_edit_context_t::create_folder(const char* name, editor_world_folder_handle_t parent_handle)
	{
		return create_folder(editor_world_controller_t::get().get_main_world_handle(), name, parent_handle);
	}

	editor_world_folder_handle_t editor_commands_world_edit_context_t::create_folder(editor_world_handle_t context, const char* name, editor_world_folder_handle_t parent_handle)
	{
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		SFG_ASSERT(parent_handle.is_null() || metadata.is_folder_valid(parent_handle));

		editor_command_create_world_folder_payload_t payload = {};
		copy_name(payload.name, name);
		payload.color			   = color_t::from255(245.0f, 194.0f, 82.0f, 255.0f);
		payload.context			   = context;
		payload.parent_folder_guid = get_folder_guid(metadata, parent_handle);

		editor_command_system_t&		  command_system = editor_command_system_t::get();
		const editor_command_issue_desc_t desc{
			.undo			   = create_world_folder_undo,
			.redo			   = create_world_folder_redo,
			.debug_name		   = "Create Folder",
			.type			   = editor_command_type_e::world_edit_context_create_folder,
			.world			   = context,
			.entity_generation = true,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue create folder command");
			return {};
		}

		editor_command_t&									command		   = command_system.get_command(handle);
		const editor_command_create_world_folder_payload_t& stored_payload = command_system.get_payload_as<editor_command_create_world_folder_payload_t>(command);
		return metadata.get_folder_handle(stored_payload.folder_guid);
	}

	bool editor_commands_world_edit_context_t::rename_folder(editor_world_folder_handle_t handle, const char* name)
	{
		return rename_folder(editor_world_controller_t::get().get_main_world_handle(), handle, name);
	}

	bool editor_commands_world_edit_context_t::rename_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, const char* name)
	{
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		if (!metadata.is_folder_valid(handle))
			return false;

		editor_command_rename_world_folder_payload_t payload = {};
		copy_name(payload.old_name, metadata.get_folder(handle).name);
		copy_name(payload.new_name, name);
		payload.context		= context;
		payload.folder_guid = metadata.get_folder(handle).guid;

		const editor_command_issue_desc_t desc{
			.undo			   = rename_world_folder_undo,
			.redo			   = rename_world_folder_redo,
			.debug_name		   = "Rename Folder",
			.type			   = editor_command_type_e::world_edit_context_rename_folder,
			.world			   = context,
			.entity_generation = true,
		};

		return !editor_command_system_t::get().issue_command(desc, payload).is_null();
	}

	bool editor_commands_world_edit_context_t::change_folder_color(editor_world_folder_handle_t handle, color_t color)
	{
		return change_folder_color(editor_world_controller_t::get().get_main_world_handle(), handle, color);
	}

	bool editor_commands_world_edit_context_t::change_folder_color(editor_world_handle_t context, editor_world_folder_handle_t handle, color_t color)
	{
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		if (!metadata.is_folder_valid(handle))
			return false;

		editor_command_color_world_folder_payload_t payload = {};
		payload.old_color									= metadata.get_folder(handle).color;
		payload.new_color									= color;
		payload.context										= context;
		payload.folder_guid									= metadata.get_folder(handle).guid;

		const editor_command_issue_desc_t desc{
			.undo			   = color_world_folder_undo,
			.redo			   = color_world_folder_redo,
			.debug_name		   = "Change Folder Color",
			.type			   = editor_command_type_e::world_edit_context_color_folder,
			.world			   = context,
			.entity_generation = true,
		};

		return !editor_command_system_t::get().issue_command(desc, payload).is_null();
	}

	bool editor_commands_world_edit_context_t::delete_folder(editor_world_folder_handle_t handle)
	{
		return delete_folder(editor_world_controller_t::get().get_main_world_handle(), handle);
	}

	bool editor_commands_world_edit_context_t::delete_folder(editor_world_handle_t context, editor_world_folder_handle_t handle)
	{
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		if (!metadata.is_folder_valid(handle))
			return false;

		vector_t<editor_world_folder_handle_t> folder_handles;
		metadata.collect_folder_tree(handle, folder_handles);

		u32 entity_guid_count = 0;
		for (editor_world_folder_handle_t folder_handle : folder_handles)
			entity_guid_count += static_cast<u32>(metadata.get_folder(folder_handle).entity_guids.size());

		editor_command_system_t& command_system = editor_command_system_t::get();

		deleted_world_folder_t*						 deleted_folders = nullptr;
		entity_guid_t*								 entity_guids	 = nullptr;
		editor_command_delete_world_folder_payload_t payload		 = {};
		payload.folders												 = command_system.get_aux_data().allocate<deleted_world_folder_t>(folder_handles.size(), deleted_folders);
		payload.context												 = context;
		payload.root_folder_guid									 = metadata.get_folder(handle).guid;
		payload.folder_count										 = static_cast<u32>(folder_handles.size());
		payload.entity_guid_count									 = entity_guid_count;
		if (entity_guid_count != 0)
			payload.entity_guids = command_system.get_aux_data().allocate<entity_guid_t>(entity_guid_count, entity_guids);

		u32 entity_guid_offset = 0;
		for (size_t i = 0; i < folder_handles.size(); ++i)
		{
			const editor_world_folder_t& folder	 = metadata.get_folder(folder_handles[i]);
			deleted_world_folder_t&		 deleted = deleted_folders[i];
			copy_name(deleted.name, folder.name);
			deleted.color[0]		   = folder.color.x;
			deleted.color[1]		   = folder.color.y;
			deleted.color[2]		   = folder.color.z;
			deleted.color[3]		   = folder.color.w;
			deleted.folder_guid		   = folder.guid;
			deleted.parent_folder_guid = get_folder_guid(metadata, folder.parent_handle);
			deleted.first_entity_guid  = entity_guid_offset;
			deleted.entity_guid_count  = static_cast<u32>(folder.entity_guids.size());
			deleted.folded			   = folder.folded;
			for (entity_guid_t guid : folder.entity_guids)
				entity_guids[entity_guid_offset++] = guid;
		}

		const editor_command_issue_desc_t desc{
			.undo			   = delete_world_folder_undo,
			.redo			   = delete_world_folder_redo,
			.cleanup		   = delete_world_folder_cleanup,
			.debug_name		   = "Delete Folder",
			.type			   = editor_command_type_e::world_edit_context_delete_folder,
			.world			   = context,
			.entity_generation = true,
		};

		const editor_command_handle_t command = command_system.issue_command(desc, payload);
		if (command.is_null())
		{
			command_system.get_aux_data().free(payload.folders);
			if (payload.entity_guids)
				command_system.get_aux_data().free(payload.entity_guids);
			return false;
		}
		return true;
	}

	bool editor_commands_world_edit_context_t::assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids)
	{
		return assign_entities_to_folder(editor_world_controller_t::get().get_main_world_handle(), handle, entity_guids);
	}

	bool editor_commands_world_edit_context_t::assign_entities_to_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids)
	{
		if (entity_guids.size == 0)
			return false;

		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		SFG_ASSERT(handle.is_null() || metadata.is_folder_valid(handle));
		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_assign_world_folder_payload_t payload = {};
		payload.entity_guids								 = copy_entity_guids_to_aux(command_system, entity_guids);
		payload.previous_folder_guids						 = copy_previous_folder_guids_to_aux(command_system, metadata, entity_guids);
		payload.context										 = context;
		payload.target_folder_guid							 = get_folder_guid(metadata, handle);
		payload.count										 = static_cast<u32>(entity_guids.size);

		const editor_command_issue_desc_t desc{
			.undo			   = assign_world_folder_undo,
			.redo			   = assign_world_folder_redo,
			.cleanup		   = assign_world_folder_cleanup,
			.debug_name		   = "Assign Folder",
			.type			   = editor_command_type_e::world_edit_context_assign_folder,
			.world			   = context,
			.entity_generation = true,
		};

		return !command_system.issue_command(desc, payload).is_null();
	}

	bool editor_commands_world_edit_context_t::deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids)
	{
		return assign_entities_to_folder({}, entity_guids);
	}

	bool editor_commands_world_edit_context_t::deassign_entities_from_folder(editor_world_handle_t context, span_t<const entity_guid_t> entity_guids)
	{
		return assign_entities_to_folder(context, {}, entity_guids);
	}

	bool editor_commands_world_edit_context_t::assign_folder_to_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle)
	{
		return assign_folder_to_folder(editor_world_controller_t::get().get_main_world_handle(), handle, parent_handle);
	}

	bool editor_commands_world_edit_context_t::assign_folder_to_folder(editor_world_handle_t context, editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle)
	{
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(context)->get_edit_context();
		if (!metadata.can_assign_folder(handle, parent_handle))
			return false;

		editor_command_assign_world_folder_parent_payload_t payload = {};
		payload.context												= context;
		payload.folder_guid											= metadata.get_folder(handle).guid;
		payload.previous_parent_guid								= get_folder_guid(metadata, metadata.get_folder(handle).parent_handle);
		payload.next_parent_guid									= get_folder_guid(metadata, parent_handle);

		const editor_command_issue_desc_t desc{
			.undo			   = assign_world_folder_parent_undo,
			.redo			   = assign_world_folder_parent_redo,
			.debug_name		   = "Assign Folder Parent",
			.type			   = editor_command_type_e::world_edit_context_assign_folder_parent,
			.world			   = context,
			.entity_generation = true,
		};

		return !editor_command_system_t::get().issue_command(desc, payload).is_null();
	}
}
