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
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_selected_entities.size() + _visible_entity_count);
		for (entity_id_t selected : _selected_entities)
			selection.push_back(selected);

		entity_id_t anchor = _selection_anchor;
		if (entity == NULL_ENTITY_ID)
		{
			selection.resize(0);
			anchor = NULL_ENTITY_ID;
		}
		else if (range_select && _selection_anchor != NULL_ENTITY_ID)
		{
			if (!incremental_select)
				selection.resize(0);

			const size_t anchor_index = find_visible_entity_index(_selection_anchor);
			const size_t entity_index = find_visible_entity_index(entity);
			if (anchor_index != SIZE_MAX && entity_index != SIZE_MAX)
			{
				const size_t first = std::min(anchor_index, entity_index);
				const size_t last  = std::max(anchor_index, entity_index);
				for (size_t i = first; i <= last; ++i)
				{
					if (std::find(selection.begin(), selection.end(), _entity_rows[i].entity) == selection.end())
						selection.push_back(_entity_rows[i].entity);
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

		issue_entity_selection_command({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : anchor);
	}

	void editor_panel_entities_t::issue_entity_selection_command(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		bool same_selection = _selected_entities.size() == entities.size;
		for (size_t i = 0; same_selection && i < entities.size; ++i)
			same_selection = _selected_entities[i] == entities.data[i];
		if (same_selection && _selection_anchor == anchor)
			return;

		SFG_ASSERT(_selected_entities.size() <= UINT32_MAX);
		SFG_ASSERT(entities.size <= UINT32_MAX);

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_entity_selection_payload_t payload = {};
		payload.previous_anchor							  = _selection_anchor;
		payload.next_anchor								  = anchor;
		payload.previous_count							  = static_cast<u32>(_selected_entities.size());
		payload.next_count								  = static_cast<u32>(entities.size);

		if (payload.previous_count != 0)
		{
			entity_id_t* previous	  = nullptr;
			payload.previous_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.previous_count, previous);
			SFG_MEMCPY(previous, _selected_entities.data(), sizeof(entity_id_t) * payload.previous_count);
		}

		if (payload.next_count != 0)
		{
			entity_id_t* next	  = nullptr;
			payload.next_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.next_count, next);
			SFG_MEMCPY(next, entities.data, sizeof(entity_id_t) * payload.next_count);
		}

		const editor_command_issue_desc_t desc{
			.undo		= on_entity_selection_undo,
			.redo		= on_entity_selection_redo,
			.cleanup	= on_entity_selection_cleanup,
			.debug_name = "Select Entities",
			.type		= editor_command_type_e::entity_selection,
		};
		command_system.issue_command(desc, payload);
	}

	void editor_panel_entities_t::apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		_selected_entities.resize(0);
		_selected_entities.reserve(entities.size);
		for (size_t i = 0; i < entities.size; ++i)
			_selected_entities.push_back(entities.data[i]);
		_selection_anchor = anchor;
		for (const entity_row_t& row : _entity_rows)
			update_entity_row_background(row);
		refresh_panel_inspector();
	}

	void editor_panel_entities_t::clear_entity_selection()
	{
		issue_entity_selection_command({}, NULL_ENTITY_ID);
	}

	void editor_panel_entities_t::select_all_visible_entities()
	{
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_visible_entity_count);
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
			selection.push_back(_entity_rows[i].entity);
		issue_entity_selection_command({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : selection.back());
	}

	void editor_panel_entities_t::append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const
	{
		out_entities.resize(0);
		out_entities.reserve(_selected_entities.size());
		for (entity_id_t entity : _selected_entities)
		{
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
		for (size_t i = 0; i < _selected_entities.size();)
		{
			if (find_entity_desc(_selected_entities[i]) == nullptr)
				_selected_entities.erase(_selected_entities.begin() + i);
			else
				++i;
		}
		if (_selection_anchor != NULL_ENTITY_ID && !is_entity_selected(_selection_anchor))
			_selection_anchor = _selected_entities.empty() ? NULL_ENTITY_ID : _selected_entities.back();
	}

	void editor_panel_entities_t::toggle_entity_fold(entity_id_t entity)
	{
		auto it = std::find(_expanded_entities.begin(), _expanded_entities.end(), entity);
		if (it == _expanded_entities.end())
			_expanded_entities.push_back(entity);
		else
			_expanded_entities.erase(it);
		refresh_entities();
	}

}
