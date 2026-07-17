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
#include "ui/widgets/outliner/editor_widget_outliner.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/outliner/editor_widget_outliner_internal.hpp"
#include <sfg/math/rectf.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	bool editor_widget_outliner_t::is_entity_expanded(entity_id_t entity) const
	{
		if (entity == NULL_ENTITY_ID || _edit_world.is_null())
			return false;

		const world_t& world = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		return editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().is_entity_expanded(world.get_entity_guid(entity));
	}

	bool editor_widget_outliner_t::is_entity_selected(entity_id_t entity) const
	{
		const span_t<const entity_id_t> selected = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().get_selected_entities();
		if (entity == NULL_ENTITY_ID || selected.size == 0)
			return false;
		return std::find(selected.data, selected.data + selected.size, entity) != selected.data + selected.size;
	}

	bool editor_widget_outliner_t::is_create_enabled() const
	{
		const editor_world_t* editor_world = editor_world_controller_t::get().get_editor_world(_edit_world);
		return editor_world->get_edit_context().get_selected_entities().size <= 1 && (_action_menu_entity == NULL_ENTITY_ID || editor_world->get_edit_context().is_entity_child_insertion_allowed(editor_world->get_world(), _action_menu_entity));
	}

	bool editor_widget_outliner_t::can_reparent_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent) const
	{
		if (entities.empty())
			return false;

		if (_edit_world.is_null())
			return false;

		editor_world_t* editor_world = editor_world_controller_t::get().get_editor_world(_edit_world);
		world_t&		world		 = editor_world->get_world();
		if (parent != NULL_ENTITY_ID && !world.is_alive(parent))
			return false;
		if (parent != NULL_ENTITY_ID && !editor_world->get_edit_context().is_entity_child_insertion_allowed(world, parent))
			return false;

		for (const editor_entity_payload_t& payload_entity : entities)
		{
			if (!(payload_entity.world == _edit_world) || payload_entity.entity == NULL_ENTITY_ID || !world.is_alive(payload_entity.entity))
				return false;
			if (!editor_world->get_edit_context().is_entity_mutation_allowed(world, payload_entity.entity))
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

	size_t editor_widget_outliner_t::find_visible_entity_index(entity_id_t entity) const
	{
		const vector_t<editor_outliner_row_t>& rows = _outliner_rows;
		for (size_t i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			if (rows[i].type == editor_outliner_item_type_e::entity && rows[i].entity == entity)
				return i;
		}
		return SIZE_MAX;
	}

	const editor_outliner_item_t* editor_widget_outliner_t::find_outliner_item(entity_id_t entity) const
	{
		const span_t<editor_outliner_item_t> items = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().get_outliner_items();
		for (size_t i = 0; i < items.size; ++i)
		{
			if (items.data[i].type == editor_outliner_item_type_e::entity && items.data[i].entity == entity)
				return &items.data[i];
		}
		return nullptr;
	}

	const editor_outliner_row_t* editor_widget_outliner_t::find_row_by_widget(ui::widget_id_t id, bool match_fold_icon) const
	{
		const vector_t<editor_outliner_row_t>& rows = _outliner_rows;
		for (u32 i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			const editor_outliner_row_t& row = rows[i];
			if (row.root == id || row.label == id || row.type_icon == id || row.type_icon_text == id || row.disable_button->get_root() == id || (match_fold_icon && (row.fold_icon == id || row.fold_icon_text == id)))
				return &row;
		}
		return nullptr;
	}

	const editor_outliner_row_t* editor_widget_outliner_t::find_row_by_pos(const vec2f_t& pos) const
	{
		const ui::layout_tree_t&			   tree = _ui->get_tree();
		const vector_t<editor_outliner_row_t>& rows = _outliner_rows;
		for (u32 i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			const editor_outliner_row_t& row = rows[i];
			const ui::layout_out_t&		 out = tree.out(row.root);
			if (rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(pos))
				return &row;
		}
		return nullptr;
	}

	const editor_outliner_row_t* editor_widget_outliner_t::find_row_by_folder(editor_world_folder_handle_t folder) const
	{
		const vector_t<editor_outliner_row_t>& rows = _outliner_rows;
		for (u32 i = 0; i < _visible_entity_count && i < rows.size(); ++i)
		{
			const editor_outliner_row_t& row = rows[i];
			if (row.type == editor_outliner_item_type_e::folder && row.folder_handle == folder)
				return &row;
		}
		return nullptr;
	}

}
