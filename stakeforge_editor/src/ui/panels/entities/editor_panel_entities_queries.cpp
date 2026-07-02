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
	bool editor_panel_entities_t::is_entity_expanded(entity_id_t entity) const
	{
		return std::find(_expanded_entities.begin(), _expanded_entities.end(), entity) != _expanded_entities.end();
	}

	bool editor_panel_entities_t::is_entity_selected(entity_id_t entity) const
	{
		return entity != NULL_ENTITY_ID && std::find(_selected_entities.begin(), _selected_entities.end(), entity) != _selected_entities.end();
	}

	bool editor_panel_entities_t::is_create_enabled() const
	{
		return _selected_entities.size() <= 1;
	}

	bool editor_panel_entities_t::has_selected_ancestor(entity_id_t entity) const
	{
		const entity_desc_t* desc = find_entity_desc(entity);
		while (desc != nullptr && desc->parent != NULL_ENTITY_ID)
		{
			if (is_entity_selected(desc->parent))
				return true;
			desc = find_entity_desc(desc->parent);
		}
		return false;
	}

	bool editor_panel_entities_t::can_reparent_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent) const
	{
		if (entities.empty())
			return false;

		const world_handle_t main_world = editor_app_t::get().get_main_world();
		if (main_world.is_null())
			return false;

		world_t& world = editor_app_t::get().get_runtime().get_world(main_world);
		if (parent != NULL_ENTITY_ID && !world.is_alive(parent))
			return false;

		for (const editor_entity_payload_t& payload_entity : entities)
		{
			if (!(payload_entity.world == main_world) || payload_entity.entity == NULL_ENTITY_ID || !world.is_alive(payload_entity.entity))
				return false;
			if (payload_entity.entity == parent)
				return false;

			for (entity_id_t cur = parent; cur != NULL_ENTITY_ID; cur = world.get_entity_parent(cur))
			{
				if (cur == payload_entity.entity)
					return false;
			}
		}
		return true;
	}

	size_t editor_panel_entities_t::find_visible_entity_index(entity_id_t entity) const
	{
		for (size_t i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			if (_entity_rows[i].entity == entity)
				return i;
		}
		return SIZE_MAX;
	}

	const editor_panel_entities_t::entity_desc_t* editor_panel_entities_t::find_entity_desc(entity_id_t entity) const
	{
		for (const entity_desc_t& desc : _entity_cache)
		{
			if (desc.id == entity)
				return &desc;
		}
		return nullptr;
	}

	const editor_panel_entities_t::entity_row_t* editor_panel_entities_t::find_row_by_widget(ui::widget_id_t id, bool match_icon) const
	{
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			const entity_row_t& row = _entity_rows[i];
			if (row.root == id || row.label == id || (match_icon && (row.icon == id || row.icon_text == id)))
				return &row;
		}
		return nullptr;
	}

	const editor_panel_entities_t::entity_row_t* editor_panel_entities_t::find_row_by_pos(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			const entity_row_t&		row = _entity_rows[i];
			const ui::layout_out_t& out = tree.out(row.root);
			if (rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(pos))
				return &row;
		}
		return nullptr;
	}

}
