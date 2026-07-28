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

	void editor_world_edit_context_t::collect_outliner_items(const world_t& world)
	{
		_outliner_items.resize(0);

		const ecs_component_table_t* alive_table	 = world.find_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t* name_table		 = world.find_component_table(type_id_t<component_name_t>::value);
		const ecs_component_table_t* disabled_table	 = world.find_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t* prefab_table	 = world.find_component_table(type_id_t<component_prefab_reference_t>::value);

		const outliner_component_tables_t tables{
			.hierarchy = hierarchy_table,
			.name	   = name_table,
			.disabled  = disabled_table,
			.prefab	   = prefab_table,
		};

		for (auto it = _folders.begin_handle(); it != _folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t handle = *it;
			if (_folders.get(handle).parent_handle.is_null())
				append_folder_items(world, tables, handle, 0);
		}

		const ecs_component_table_ref_t table_refs[] = {
			alive_table->ref(),
			hierarchy_table->ref(),
			name_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::row_get<component_hierarchy_t>(row, 1);
			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			if (is_entity_assigned(world.get_entity_guid(row.id)))
				continue;

			append_entity_items(world, tables, row.id, 0);
		}
	}

	void editor_world_edit_context_t::set_entity_folded(entity_guid_t guid, bool folded)
	{
		get_or_create_entity_metadata(guid).folded = folded;
	}

	bool editor_world_edit_context_t::is_entity_expanded(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr && !meta->folded;
	}

	editor_world_entity_metadata_t& editor_world_edit_context_t::get_or_create_entity_metadata(entity_guid_t guid)
	{
		if (editor_world_entity_metadata_t* meta = find_entity_metadata(guid))
			return *meta;

		_entity_metadata.push_back({.guid = guid});
		return _entity_metadata.back();
	}

	editor_world_entity_metadata_t* editor_world_edit_context_t::find_entity_metadata(entity_guid_t guid)
	{
		for (editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}
		return nullptr;
	}

	const editor_world_entity_metadata_t* editor_world_edit_context_t::find_entity_metadata(entity_guid_t guid) const
	{
		for (const editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}
		return nullptr;
	}

	void editor_world_edit_context_t::append_folder_items(const world_t& world, const outliner_component_tables_t& tables, editor_world_folder_handle_t handle, u16 depth)
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
				append_folder_items(world, tables, child_handle, static_cast<u16>(depth + 1));
		}

		for (entity_guid_t guid : folder.entity_guids)
		{
			const entity_id_t entity = world.find_by_guid(guid);
			if (entity != NULL_ENTITY_ID && world.is_alive(entity))
				append_entity_items(world, tables, entity, static_cast<u16>(depth + 1));
		}
	}

	void editor_world_edit_context_t::append_entity_items(const world_t& world, const outliner_component_tables_t& tables, entity_id_t id, u16 depth)
	{
		if (id == _editor_camera_entity)
			return;

		const component_hierarchy_t& hierarchy		  = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*tables.hierarchy, id);
		const component_name_t&		 name			  = ecs_helpers_t::table_get_as_const<component_name_t>(*tables.name, id);
		const entity_guid_t			 guid			  = world.get_entity_guid(id);
		const bool					 disabled		  = ecs_t::table_has(*tables.disabled, id);
		const bool					 prefab_reference = ecs_t::table_has(*tables.prefab, id);

		_outliner_items.push_back({
			.name				  = name.text,
			.entity				  = id,
			.parent_entity		  = hierarchy.parent,
			.entity_guid		  = guid,
			.depth				  = depth,
			.type				  = editor_outliner_item_type_e::entity,
			.has_children		  = hierarchy.first_child != NULL_ENTITY_ID,
			.selected			  = is_entity_selected(id),
			.disabled			  = disabled,
			.has_prefab_reference = prefab_reference,
		});
		if (hierarchy.first_child == NULL_ENTITY_ID)
			return;

		entity_id_t child = hierarchy.first_child;
		while (child != NULL_ENTITY_ID)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*tables.hierarchy, child);
			append_entity_items(world, tables, child, static_cast<u16>(depth + 1));
			child = child_hierarchy.next_sibling;
		}
	}

	bool editor_world_edit_context_t::is_entity_assigned(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);
		return meta != nullptr && !meta->folder_handle.is_null() && _folders.is_valid(meta->folder_handle);
	}

	bool editor_world_edit_context_t::is_entity_selected(entity_id_t entity) const
	{
		return std::find(_selected_entities.begin(), _selected_entities.end(), entity) != _selected_entities.end();
	}

}
