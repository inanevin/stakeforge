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
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	void editor_panel_entities_t::select_entity_row(entity_id_t entity, bool range_select, bool incremental_select)
	{
		editor_selection_controller_t&	controller = editor_app_t::get().get_selection_controller();
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

			const vector_t<editor_outliner_row_t>& rows			= editor_app_t::get().get_world_metadata().get_outliner_rows();
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

	void editor_panel_entities_t::select_all_visible_entities()
	{
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_visible_entity_count);
		const vector_t<editor_outliner_row_t>& rows = editor_app_t::get().get_world_metadata().get_outliner_rows();
		for (u32 i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			if (rows[i].type == editor_outliner_item_type_e::entity)
				selection.push_back(rows[i].entity);
		}
		editor_app_t::get().get_selection_controller().issue_entity_selection({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : selection.back());
	}

	void editor_panel_entities_t::append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const
	{
		const span_t<const entity_id_t> selected = editor_app_t::get().get_selection_controller().get_selected_entities();
		out_entities.resize(0);
		out_entities.reserve(selected.size);
		for (size_t i = 0; i < selected.size; ++i)
		{
			const entity_id_t entity = selected.data[i];
			if (!has_selected_ancestor(entity))
				out_entities.push_back(entity);
		}
	}

	void editor_panel_entities_t::collect_payload_entities(entity_id_t entity)
	{
		_payload_entities.resize(0);

		const world_handle_t main_world = editor_app_t::get().get_main_world();
		if (main_world.is_null())
			return;

		world_t& world = editor_app_t::get().get_runtime().get_world(main_world);
		if (is_entity_selected(entity))
		{
			frame_vector_t<entity_id_t> root_entities;
			append_selected_root_entities(root_entities);
			for (entity_id_t selected_entity : root_entities)
			{
				if (selected_entity != NULL_ENTITY_ID && world.is_alive(selected_entity))
					_payload_entities.push_back({.world = main_world, .entity = selected_entity});
			}
		}

		if (_payload_entities.empty() && entity != NULL_ENTITY_ID && world.is_alive(entity))
			_payload_entities.push_back({.world = main_world, .entity = entity});
	}

	void editor_panel_entities_t::prune_entity_selection()
	{
		editor_selection_controller_t&	controller = editor_app_t::get().get_selection_controller();
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

	bool editor_panel_entities_t::reveal_entity(entity_id_t entity)
	{
		if (entity == NULL_ENTITY_ID || _main_world.is_null())
			return false;

		world_t& world = editor_app_t::get().get_runtime().get_world(_main_world);
		if (!world.is_alive(entity))
			return false;

		editor_world_metadata_t& metadata = editor_app_t::get().get_world_metadata();
		bool					 changed  = false;
		entity_id_t				 root	  = entity;
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

	void editor_panel_entities_t::toggle_entity_fold(entity_id_t entity)
	{
		world_t&				 world	  = editor_app_t::get().get_runtime().get_world(_main_world);
		editor_world_metadata_t& metadata = editor_app_t::get().get_world_metadata();
		const entity_guid_t		 guid	  = world.get_entity_guid(entity);
		metadata.set_entity_folded(guid, metadata.is_entity_expanded(guid));
		refresh_entities();
	}

}
