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
#include "editor_widget_outliner.hpp"
#include "editor/ui/widgets/editor_widgets_icons.hpp"
#include <sfg/core/runtime/world/ecs.hpp>
#include <sfg/core/runtime/world/engine_components.hpp>
#include <sfg/core/runtime/world/world.hpp>

namespace sfg
{
	void editor_widget_outliner_t::collect_entities(const world_t& world, const editor_world_edit_context_t& context)
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

		const auto& folders = context.get_folders();

		for (auto it = folders.begin_handle(); it != folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t handle = *it;

			if (folders.get(handle).parent_handle.is_null())
				append_folder_items(world, context, tables, handle, 0);
		}

		const ecs_component_table_ref_t table_refs[] = {
			alive_table->ref(),
			hierarchy_table->ref(),
			name_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = row.get<component_hierarchy_t>(1);

			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			const editor_world_folder_handle_t folder = context.get_entity_folder(world.get_entity_guid(row.id));

			if (!folder.is_null() && folders.is_valid(folder))
				continue;

			append_entity_items(world, context, tables, row.id, 0);
		}
	}

	void editor_widget_outliner_t::append_folder_items(const world_t& world, const editor_world_edit_context_t& context, const outliner_component_tables_t& tables, editor_world_folder_handle_t handle, u16 depth)
	{
		const auto&					 folders	  = context.get_folders();
		const editor_world_folder_t& folder		  = folders.get(handle);
		bool						 has_children = !folder.entity_guids.empty();

		for (auto it = folders.begin_handle(); !has_children && it != folders.end_handle(); ++it)
			has_children = folders.get(*it).parent_handle == handle;

		_outliner_items.push_back({.name = folder.name, .type_icon = ICON_FOLDER, .folder_handle = handle, .color = folder.color, .depth = depth, .type = editor_outliner_item_type_e::folder, .has_children = has_children});

		for (auto it = folders.begin_handle(); it != folders.end_handle(); ++it)
		{
			const editor_world_folder_handle_t child_handle = *it;

			if (folders.get(child_handle).parent_handle == handle)
				append_folder_items(world, context, tables, child_handle, static_cast<u16>(depth + 1));
		}

		for (entity_guid_t guid : folder.entity_guids)
		{
			const entity_id_t entity = world.find_by_guid(guid);

			if (entity != NULL_ENTITY_ID && world.is_alive(entity))
				append_entity_items(world, context, tables, entity, static_cast<u16>(depth + 1));
		}
	}

	void editor_widget_outliner_t::append_entity_items(const world_t& world, const editor_world_edit_context_t& context, const outliner_component_tables_t& tables, entity_id_t id, u16 depth)
	{
		if (id == context.get_editor_camera_entity())
			return;

		const component_hierarchy_t& hierarchy		  = tables.hierarchy->get_as_const<component_hierarchy_t>(id);
		const component_name_t&		 name			  = tables.name->get_as_const<component_name_t>(id);
		const entity_guid_t			 guid			  = world.get_entity_guid(id);
		const bool					 disabled		  = tables.disabled->has(id);
		const bool					 prefab_reference = tables.prefab->has(id);

		_outliner_items.push_back({
			.name				  = name.text,
			.entity				  = id,
			.entity_guid		  = guid,
			.depth				  = depth,
			.type				  = editor_outliner_item_type_e::entity,
			.has_children		  = hierarchy.first_child != NULL_ENTITY_ID,
			.disabled			  = disabled,
			.has_prefab_reference = prefab_reference,
		});

		if (hierarchy.first_child == NULL_ENTITY_ID)
			return;

		entity_id_t child = hierarchy.first_child;

		while (child != NULL_ENTITY_ID)
		{
			const component_hierarchy_t& child_hierarchy = tables.hierarchy->get_as_const<component_hierarchy_t>(child);

			append_entity_items(world, context, tables, child, static_cast<u16>(depth + 1));
			child = child_hierarchy.next_sibling;
		}
	}

}
