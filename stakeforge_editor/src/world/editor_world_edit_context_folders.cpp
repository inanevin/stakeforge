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
#include "world/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY 64
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS			  1024
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS 64

	editor_world_folder_handle_t editor_world_edit_context_t::create_folder(const char* name)
	{
		return create_folder_with_guid(name, color_t::from255(245.0f, 194.0f, 82.0f, 255.0f), false, {}, _next_guid++);
	}

	editor_world_folder_handle_t editor_world_edit_context_t::create_folder_with_guid(const char* name, color_t color, bool folded, editor_world_folder_handle_t parent_handle, u64 guid)
	{
		SFG_ASSERT(_folders.is_valid(parent_handle));

		if (guid == 0)
			guid = _next_guid++;

		const editor_world_folder_handle_t handle = _folders.emplace();
		editor_world_folder_t&			   folder = _folders.get(handle);
		folder.name_storage						  = name != nullptr ? name : "Folder";
		folder.name								  = folder.name_storage.c_str();
		folder.color							  = color;
		folder.guid								  = guid;
		folder.parent_handle					  = parent_handle;
		folder.folded							  = folded;
		_next_guid								  = guid >= _next_guid ? guid + 1 : _next_guid;
		return handle;
	}

	void editor_world_edit_context_t::destroy_folder(editor_world_folder_handle_t handle)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		editor_world_folder_t& folder = _folders.get(handle);
		for (entity_guid_t guid : folder.entity_guids)
		{
			editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
			if (meta != nullptr && meta->folder_handle == handle)
				meta->folder_handle = {};
		}

		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t folder_handle = *it;
			if (folder_handle == handle)
				continue;

			editor_world_folder_t& child = _folders.get(folder_handle);
			if (child.parent_handle == handle)
				child.parent_handle = {};
		}

		_folders.remove(handle);
	}

	void editor_world_edit_context_t::set_folder_name(editor_world_folder_handle_t handle, const char* name)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		editor_world_folder_t& folder = _folders.get(handle);
		folder.name_storage			  = name != nullptr ? name : "";
		folder.name					  = folder.name_storage.c_str();
	}

	void editor_world_edit_context_t::set_folder_color(editor_world_folder_handle_t handle, color_t color)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		_folders.get(handle).color = color;
	}

	void editor_world_edit_context_t::set_folder_folded(editor_world_folder_handle_t handle, bool folded)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		_folders.get(handle).folded = folded;
	}

	void editor_world_edit_context_t::set_folder_parent(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle)
	{
		SFG_ASSERT(can_assign_folder(handle, parent_handle));
		_folders.get(handle).parent_handle = parent_handle;
	}

	void editor_world_edit_context_t::assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids)
	{
		SFG_ASSERT(handle.is_null() || _folders.is_valid(handle));
		for (size_t i = 0; i < entity_guids.size; ++i)
		{
			const entity_guid_t guid = entity_guids.data[i];
			remove_entity_from_folders(guid);
			editor_world_entity_metadata_t& meta = get_or_create_entity_metadata(guid);
			meta.folder_handle					 = handle;
			if (!handle.is_null())
				_folders.get(handle).entity_guids.push_back(guid);
		}
	}

	void editor_world_edit_context_t::deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids)
	{
		for (size_t i = 0; i < entity_guids.size; ++i)
		{
			remove_entity_from_folders(entity_guids.data[i]);
			editor_world_entity_metadata_t& meta = get_or_create_entity_metadata(entity_guids.data[i]);
			meta.folder_handle					 = {};
		}
	}

	void editor_world_edit_context_t::write_folders_to_json(nlohmann::json& out_json) const
	{
		out_json = nlohmann::json::array();
		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_t& folder		  = _folders.get(*it);
			nlohmann::json				 entity_guids = nlohmann::json::array();
			for (entity_guid_t guid : folder.entity_guids)
				entity_guids.push_back(guid);

			out_json.push_back({
				{"guid", folder.guid},
				{"parent_guid", folder.parent_handle.is_null() ? 0 : _folders.get(folder.parent_handle).guid},
				{"name", folder.name != nullptr ? folder.name : ""},
				{"color", nlohmann::json{{"x", folder.color.x}, {"y", folder.color.y}, {"z", folder.color.z}, {"w", folder.color.w}}},
				{"folded", folder.folded},
				{"entities", entity_guids},
			});
		}
	}

	void editor_world_edit_context_t::read_folders_from_json(const nlohmann::json& in_json)
	{
		if (!in_json.is_array())
			return;

		for (const nlohmann::json& folder_json : in_json)
		{
			const nlohmann::json		   color_json = folder_json.value<nlohmann::json>("color", nlohmann::json::object());
			const nlohmann::json::string_t name		  = folder_json.value<nlohmann::json::string_t>("name", "Folder");
			create_folder_with_guid(
				name.c_str(), color_t(color_json.value<f32>("x", 1.0f), color_json.value<f32>("y", 1.0f), color_json.value<f32>("z", 1.0f), color_json.value<f32>("w", 1.0f)), folder_json.value<bool>("folded", false), {}, folder_json.value<u64>("guid", 0));
		}

		for (const nlohmann::json& folder_json : in_json)
		{
			const editor_world_folder_handle_t handle = get_folder_handle(folder_json.value<u64>("guid", 0));
			if (handle.is_null())
				continue;

			const editor_world_folder_handle_t parent_handle = get_folder_handle(folder_json.value<u64>("parent_guid", 0));
			if (!parent_handle.is_null() && can_assign_folder(handle, parent_handle))
				set_folder_parent(handle, parent_handle);

			const nlohmann::json entity_guids_json = folder_json.value<nlohmann::json>("entities", nlohmann::json::array());
			if (!entity_guids_json.is_array())
				continue;

			for (const nlohmann::json& entity_guid_json : entity_guids_json)
			{
				const entity_guid_t guid = entity_guid_json.get<entity_guid_t>();
				assign_entities_to_folder(handle, {.data = &guid, .size = 1});
			}
		}
	}

	editor_world_folder_handle_t editor_world_edit_context_t::get_folder_handle(u64 guid) const
	{
		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t handle = *it;
			if (_folders.get(handle).guid == guid)
				return handle;
		}
		return {};
	}

	editor_world_folder_handle_t editor_world_edit_context_t::get_entity_folder(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr ? meta->folder_handle : editor_world_folder_handle_t{};
	}

	bool editor_world_edit_context_t::can_assign_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle) const
	{
		if (!_folders.is_valid(handle))
			return false;
		if (parent_handle.is_null())
			return true;
		if (!_folders.is_valid(parent_handle) || handle == parent_handle)
			return false;
		for (editor_world_folder_handle_t cur = parent_handle; !cur.is_null(); cur = _folders.get(cur).parent_handle)
		{
			if (cur == handle)
				return false;
		}
		return true;
	}

	void editor_world_edit_context_t::collect_folder_tree(editor_world_folder_handle_t handle, vector_t<editor_world_folder_handle_t>& out_handles) const
	{
		SFG_ASSERT(_folders.is_valid(handle));
		out_handles.push_back(handle);
		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t child_handle = *it;
			if (_folders.get(child_handle).parent_handle == handle)
				collect_folder_tree(child_handle, out_handles);
		}
	}

	void editor_world_edit_context_t::remove_entity_from_folders(entity_guid_t guid)
	{
		for (editor_world_folder_t& folder : _folders)
		{
			auto it = std::find(folder.entity_guids.begin(), folder.entity_guids.end(), guid);
			if (it != folder.entity_guids.end())
				folder.entity_guids.erase(it);
		}
	}

}
