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
#include "editor_world_metadata.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_world_metadata_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);
		SFG_ASSERT(!_inited);
		s_instance = this;
		_folders.init(EDITOR_WORLD_METADATA_MAX_FOLDERS);
		_entity_metadata.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_outliner_items.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_outliner_rows.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_inited = true;
	}

	void editor_world_metadata_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		SFG_ASSERT(_inited);
		_outliner_rows.clear();
		_outliner_items.clear();
		_entity_metadata.clear();
		_folders.uninit();
		_world	   = {};
		_next_guid = 1;
		_inited	   = false;
		s_instance = nullptr;
	}

	void editor_world_metadata_t::set_world(world_handle_t world)
	{
		SFG_ASSERT(_inited);
		_world = world;
		_outliner_items.resize(0);
		_entity_metadata.resize(0);
		_folders.reset();
		_next_guid = 1;
	}

	editor_world_folder_handle_t editor_world_metadata_t::create_folder(const char* name)
	{
		return create_folder_with_guid(name, color_t::from255(245.0f, 194.0f, 82.0f, 255.0f), false, {}, _next_guid++);
	}

	editor_world_folder_handle_t editor_world_metadata_t::create_folder_with_guid(const char* name, color_t color, bool folded, editor_world_folder_handle_t parent_handle, u64 guid)
	{
		SFG_ASSERT(_inited);
		SFG_ASSERT(!_folders.is_full());
		SFG_ASSERT(parent_handle.is_null() || _folders.is_valid(parent_handle));

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

	void editor_world_metadata_t::destroy_folder(editor_world_folder_handle_t handle)
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

	void editor_world_metadata_t::set_folder_name(editor_world_folder_handle_t handle, const char* name)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		editor_world_folder_t& folder = _folders.get(handle);
		folder.name_storage			  = name != nullptr ? name : "";
		folder.name					  = folder.name_storage.c_str();
	}

	void editor_world_metadata_t::set_folder_color(editor_world_folder_handle_t handle, color_t color)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		_folders.get(handle).color = color;
	}

	void editor_world_metadata_t::set_folder_folded(editor_world_folder_handle_t handle, bool folded)
	{
		SFG_ASSERT(_folders.is_valid(handle));
		_folders.get(handle).folded = folded;
	}

	void editor_world_metadata_t::set_folder_parent(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle)
	{
		SFG_ASSERT(can_assign_folder(handle, parent_handle));
		_folders.get(handle).parent_handle = parent_handle;
	}

	void editor_world_metadata_t::set_entity_folded(entity_guid_t guid, bool folded)
	{
		get_or_create_entity_metadata(guid).folded = folded;
	}

	void editor_world_metadata_t::assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids)
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

	void editor_world_metadata_t::deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids)
	{
		for (size_t i = 0; i < entity_guids.size; ++i)
		{
			remove_entity_from_folders(entity_guids.data[i]);
			editor_world_entity_metadata_t& meta = get_or_create_entity_metadata(entity_guids.data[i]);
			meta.folder_handle					 = {};
		}
	}

	void editor_world_metadata_t::collect_outliner_items(const world_t& world)
	{
		_outliner_items.resize(0);

		const world_component_table_t* alive_table	   = world.find_component_table(type_id_t<component_alive_t>::value);
		const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const world_component_table_t* name_table	   = world.find_component_table(type_id_t<component_name_t>::value);
		const world_component_table_t* disabled_table  = world.find_component_table(type_id_t<component_disabled_t>::value);
		SFG_ASSERT(alive_table != nullptr);
		SFG_ASSERT(hierarchy_table != nullptr);
		SFG_ASSERT(name_table != nullptr);
		SFG_ASSERT(disabled_table != nullptr);

		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t handle = *it;
			if (_folders.get(handle).parent_handle.is_null())
				append_folder_items(world, disabled_table->table, handle, 0);
		}

		const ecs_component_table_ref_t table_refs[] = {
			alive_table->table.ref(),
			hierarchy_table->table.ref(),
			name_table->table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::row_get<component_hierarchy_t>(row, 1);
			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			if (is_entity_assigned(world.get_entity_guid(row.id)))
				continue;

			append_entity_items(world, hierarchy_table->table, name_table->table, disabled_table->table, row.id, 0);
		}
	}

	void editor_world_metadata_t::write_folders_to_json(nlohmann::json& out_json) const
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

	void editor_world_metadata_t::read_folders_from_json(const nlohmann::json& in_json)
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

	editor_world_folder_t& editor_world_metadata_t::get_folder(editor_world_folder_handle_t handle)
	{
		return _folders.get(handle);
	}

	const editor_world_folder_t& editor_world_metadata_t::get_folder(editor_world_folder_handle_t handle) const
	{
		return _folders.get(handle);
	}

	editor_world_folder_handle_t editor_world_metadata_t::get_folder_handle(u64 guid) const
	{
		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t handle = *it;
			if (_folders.get(handle).guid == guid)
				return handle;
		}
		return {};
	}

	editor_world_folder_handle_t editor_world_metadata_t::get_entity_folder(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr ? meta->folder_handle : editor_world_folder_handle_t{};
	}

	bool editor_world_metadata_t::is_folder_valid(editor_world_folder_handle_t handle) const
	{
		return _folders.is_valid(handle);
	}

	bool editor_world_metadata_t::can_assign_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle) const
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

	bool editor_world_metadata_t::is_entity_expanded(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr && !meta->folded;
	}

	span_t<editor_outliner_item_t> editor_world_metadata_t::get_outliner_items()
	{
		return {.data = _outliner_items.data(), .size = _outliner_items.size()};
	}

	span_t<const editor_outliner_item_t> editor_world_metadata_t::get_outliner_items() const
	{
		return {.data = _outliner_items.data(), .size = _outliner_items.size()};
	}

	vector_t<editor_outliner_row_t>& editor_world_metadata_t::get_outliner_rows()
	{
		return _outliner_rows;
	}

	world_handle_t editor_world_metadata_t::get_world() const
	{
		return _world;
	}

	editor_world_entity_metadata_t& editor_world_metadata_t::get_or_create_entity_metadata(entity_guid_t guid)
	{
		if (editor_world_entity_metadata_t* meta = find_entity_metadata(guid))
			return *meta;

		_entity_metadata.push_back({.guid = guid});
		return _entity_metadata.back();
	}

	editor_world_entity_metadata_t* editor_world_metadata_t::find_entity_metadata(entity_guid_t guid)
	{
		for (editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}
		return nullptr;
	}

	const editor_world_entity_metadata_t* editor_world_metadata_t::find_entity_metadata(entity_guid_t guid) const
	{
		for (const editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}
		return nullptr;
	}

	void editor_world_metadata_t::append_folder_items(const world_t& world, const ecs_component_table_t& disabled_table, editor_world_folder_handle_t handle, u16 depth)
	{
		const editor_world_folder_t& folder		  = _folders.get(handle);
		bool						 has_children = !folder.entity_guids.empty();
		for (auto it = _folders.begin_handle(); !has_children && it != _folders.end_handle(); ++it)
			has_children = _folders.get(*it).parent_handle == handle;

		_outliner_items.push_back({.name = folder.name, .type_icon = ICON_FOLDER, .folder_handle = handle, .color = folder.color, .depth = depth, .type = editor_outliner_item_type_e::folder, .has_children = has_children});

		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t child_handle = *it;
			if (_folders.get(child_handle).parent_handle == handle)
				append_folder_items(world, disabled_table, child_handle, static_cast<u16>(depth + 1));
		}

		const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const world_component_table_t* name_table	   = world.find_component_table(type_id_t<component_name_t>::value);
		SFG_ASSERT(hierarchy_table != nullptr);
		SFG_ASSERT(name_table != nullptr);
		for (entity_guid_t guid : folder.entity_guids)
		{
			const entity_id_t entity = world.get_entity_from_guid(guid);
			if (entity != NULL_ENTITY_ID && world.is_alive(entity))
				append_entity_items(world, hierarchy_table->table, name_table->table, disabled_table, entity, static_cast<u16>(depth + 1));
		}
	}

	void editor_world_metadata_t::append_entity_items(const world_t& world, const ecs_component_table_t& hierarchy_table, const ecs_component_table_t& name_table, const ecs_component_table_t& disabled_table, entity_id_t id, u16 depth)
	{
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, id);
		const component_name_t&		 name	   = ecs_helpers_t::table_get_as_const<component_name_t>(name_table, id);
		const entity_guid_t			 guid	   = world.get_entity_guid(id);
		const bool					 disabled  = ecs_t::table_has(disabled_table, id);

		_outliner_items.push_back(
			{.name = name.text, .entity = id, .parent_entity = hierarchy.parent, .entity_guid = guid, .depth = depth, .type = editor_outliner_item_type_e::entity, .has_children = hierarchy.first_child != NULL_ENTITY_ID, .disabled = disabled});
		if (hierarchy.first_child == NULL_ENTITY_ID)
			return;

		entity_id_t child = hierarchy.first_child;
		while (child != NULL_ENTITY_ID)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
			append_entity_items(world, hierarchy_table, name_table, disabled_table, child, static_cast<u16>(depth + 1));
			child = child_hierarchy.next_sibling;
		}
	}

	bool editor_world_metadata_t::is_entity_assigned(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr && !meta->folder_handle.is_null() && _folders.is_valid(meta->folder_handle);
	}

	void editor_world_metadata_t::remove_entity_from_folders(entity_guid_t guid)
	{
		for (editor_world_folder_t& folder : _folders)
		{
			auto it = std::find(folder.entity_guids.begin(), folder.entity_guids.end(), guid);
			if (it != folder.entity_guids.end())
				folder.entity_guids.erase(it);
		}
	}
}
