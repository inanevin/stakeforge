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
#include "ui/widgets/outliner/editor_widget_outliner.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/widgets/outliner/editor_widget_outliner_internal.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_outliner_t::select_entity_row(entity_id_t entity, bool range_select, bool incremental_select)
	{
		editor_world_edit_context_t&	controller = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		const span_t<const entity_id_t> selected   = controller.get_selected_entities();

		frame_vector_t<entity_id_t> selection;
		selection.reserve(selected.size + _visible_entity_count);
		for (size_t i = 0; i < selected.size; ++i)
			selection.push_back(selected.data[i]);

		entity_id_t anchor = controller.get_entity_anchor();
		if (entity == NULL_ENTITY_ID)
		{
			selection.resize(0);
			anchor = NULL_ENTITY_ID;
		}
		else if (range_select && anchor != NULL_ENTITY_ID)
		{
			if (!incremental_select)
				selection.resize(0);

			const vector_t<editor_outliner_row_t>& rows			= _outliner_rows;
			const size_t						   anchor_index = find_visible_entity_index(anchor);
			const size_t						   entity_index = find_visible_entity_index(entity);
			if (anchor_index != SIZE_MAX && entity_index != SIZE_MAX)
			{
				const size_t first = std::min(anchor_index, entity_index);
				const size_t last  = std::max(anchor_index, entity_index);
				for (size_t i = first; i <= last; ++i)
				{
					if (rows[i].type == editor_outliner_item_type_e::entity && std::find(selection.begin(), selection.end(), rows[i].entity) == selection.end())
						selection.push_back(rows[i].entity);
				}
			}
			else
			{
				selection.resize(0);
				selection.push_back(entity);
				anchor = entity;
			}
		}
		else if (incremental_select)
		{
			auto it = std::find(selection.begin(), selection.end(), entity);
			if (it != selection.end())
				selection.erase(it);
			else
				selection.push_back(entity);
			anchor = entity;
		}
		else
		{
			selection.resize(0);
			selection.push_back(entity);
			anchor = entity;
		}

		controller.issue_entity_selection({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : anchor);
	}

	void editor_widget_outliner_t::select_all_visible_entities()
	{
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_visible_entity_count);
		const vector_t<editor_outliner_row_t>& rows = _outliner_rows;
		for (u32 i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			if (rows[i].type == editor_outliner_item_type_e::entity)
				selection.push_back(rows[i].entity);
		}
		editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().issue_entity_selection({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : selection.back());
	}

	void editor_widget_outliner_t::append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const
	{
		const editor_world_t*			editor_world = editor_world_controller_t::get().get_editor_world(_edit_world);
		const span_t<const entity_id_t> selected	 = editor_world->get_edit_context().get_selected_entities();
		out_entities.resize(selected.size);
		out_entities.resize(editor_world->get_edit_context().collect_selected_mutable_root_entities(editor_world->get_world(), {.data = out_entities.data(), .size = out_entities.size()}));
	}

	void editor_widget_outliner_t::collect_payload_entities(entity_id_t entity)
	{
		_payload_entities.resize(0);

		if (_edit_world.is_null())
			return;

		editor_world_t* editor_world = editor_world_controller_t::get().get_editor_world(_edit_world);
		world_t&		world		 = editor_world->get_world();
		if (is_entity_selected(entity))
		{
			frame_vector_t<entity_id_t> root_entities;
			append_selected_root_entities(root_entities);
			for (entity_id_t selected_entity : root_entities)
			{
				if (selected_entity != NULL_ENTITY_ID && world.is_alive(selected_entity))
					_payload_entities.push_back({.world = _edit_world, .entity = selected_entity});
			}
		}

		if (_payload_entities.empty() && entity != NULL_ENTITY_ID && world.is_alive(entity) && editor_world->get_edit_context().is_entity_mutation_allowed(world, entity))
			_payload_entities.push_back({.world = _edit_world, .entity = entity});
	}

	void editor_widget_outliner_t::prune_entity_selection()
	{
		editor_world_edit_context_t&	controller = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		const span_t<const entity_id_t> selected   = controller.get_selected_entities();
		frame_vector_t<entity_id_t>		selection;
		selection.reserve(selected.size);
		for (size_t i = 0; i < selected.size; ++i)
		{
			if (find_outliner_item(selected.data[i]) != nullptr)
				selection.push_back(selected.data[i]);
		}

		entity_id_t anchor = controller.get_entity_anchor();
		if (anchor != NULL_ENTITY_ID && std::find(selection.begin(), selection.end(), anchor) == selection.end())
			anchor = selection.empty() ? NULL_ENTITY_ID : selection.back();

		bool same_selection = selected.size == selection.size();
		for (size_t i = 0; same_selection && i < selected.size; ++i)
			same_selection = selected.data[i] == selection[i];
		if (same_selection && controller.get_entity_anchor() == anchor)
			return;

		controller.apply_entity_selection({.data = selection.data(), .size = selection.size()}, anchor);
	}

	void editor_widget_outliner_t::show_entity(entity_guid_t guid)
	{
		if (guid == NULL_ENTITY_GUID || _edit_world.is_null())
			return;

		world_t&		  world	 = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		const entity_id_t entity = world.find_by_guid(guid);
		if (entity == NULL_ENTITY_ID || !world.is_alive(entity))
			return;

		if (!_search_str.empty())
		{
			_search_str.resize(0);
			_search_str_lower.resize(0);
			_search_input.set_text("");
		}

		reveal_entity(entity);
		refresh_entities();
		select_entity_row(entity, false, false);

		const size_t entity_index = find_visible_entity_index(entity);
		if (entity_index == SIZE_MAX)
			return;

		ui::layout_tree_t&		tree		= _ui->get_tree();
		ui::layout_in_t&		list_in		= tree.in(_entity_list_area);
		const ui::layout_out_t& list_out	= tree.out(_entity_list_area);
		const editor_theme_t&	theme		= editor_theme_t::get();
		const f32				scale		= ui::get_valid_scale(_ui->get_ui_scale());
		const f32				viewport	= list_out.size.y / scale;
		const f32				row_top		= theme.margin_vertical + static_cast<f32>(entity_index) * theme.item_height;
		const f32				row_bottom	= row_top + theme.item_height;
		const f32				current_top = -list_in.scroll_offset.y;
		f32						target		= current_top;
		if (row_top < current_top)
			target = row_top;
		else if (row_bottom > current_top + viewport)
			target = row_bottom - viewport;
		list_in.scroll_offset.y = -math::max(0.0f, target);
	}

	bool editor_widget_outliner_t::reveal_entity(entity_id_t entity)
	{
		if (entity == NULL_ENTITY_ID || _edit_world.is_null())
			return false;

		world_t& world = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		if (!world.is_alive(entity))
			return false;

		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		bool						 changed  = false;
		entity_id_t					 root	  = entity;
		for (entity_id_t parent = world.get_entity_parent(entity); parent != NULL_ENTITY_ID; parent = world.get_entity_parent(parent))
		{
			const entity_guid_t guid = world.get_entity_guid(parent);
			if (!metadata.is_entity_expanded(guid))
			{
				metadata.set_entity_folded(guid, false);
				changed = true;
			}
			root = parent;
		}

		for (editor_world_folder_handle_t folder = metadata.get_entity_folder(world.get_entity_guid(root)); !folder.is_null(); folder = metadata.get_folder(folder).parent_handle)
		{
			if (metadata.get_folder(folder).folded)
			{
				metadata.set_folder_folded(folder, false);
				changed = true;
			}
		}
		return changed;
	}

	void editor_widget_outliner_t::toggle_entity_fold(entity_id_t entity)
	{
		world_t&					 world	  = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		editor_world_edit_context_t& metadata = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		const entity_guid_t			 guid	  = world.get_entity_guid(entity);
		metadata.set_entity_folded(guid, metadata.is_entity_expanded(guid));
		refresh_entities();
	}

}
