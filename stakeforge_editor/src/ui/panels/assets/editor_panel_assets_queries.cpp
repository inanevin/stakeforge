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
#include "ui/panels/assets/editor_panel_assets.hpp"
#include "ui/panels/assets/editor_panel_assets_internal.hpp"
#include "assets/editor_asset_creator.hpp"
#include "assets/editor_asset_importer.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_string.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>
namespace sfg
{
	string_t editor_panel_assets_t::get_action_menu_target_folder_path() const
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 tree		   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t target		   = !_selected_folder_node.is_null() ? _selected_folder_node : _selected_folder_hash == 0 ? asset_manager.get_root_node() : editor_asset_node_handle_t{};
		if (!target.is_null() && tree.is_valid(target))
		{
			const string_t& path = tree.value(target).full_path;
			if (!path.empty())
				return path;
		}

		return {};
	}

	u64 editor_panel_assets_t::get_folder_hash_after_rename(editor_asset_node_handle_t node, const string_t& name) const
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t root_handle   = asset_manager.get_root_node();
		SFG_ASSERT(!node.is_null());
		SFG_ASSERT(!root_handle.is_null());
		SFG_ASSERT(asset_tree.is_valid(node));

		frame_vector_t<editor_asset_node_handle_t> chain;
		editor_asset_node_handle_t				   current = node;
		while (!current.is_null() && !(current == root_handle))
		{
			chain.push_back(current);
			current = asset_tree.parent(current);
		}

		const editor_asset_node_t& root		 = asset_tree.value(root_handle);
		const string_t&			   root_name = node == root_handle ? name : root.name;

		frame_string_t<char> relative_path;
		relative_path.assign(root_name.c_str(), root_name.size());
		for (size_t i = chain.size(); i-- > 0;)
		{
			const editor_asset_node_t& chain_node = asset_tree.value(chain[i]);
			const string_t&			   segment	  = chain[i] == node ? name : chain_node.name;
			relative_path += '/';
			relative_path.append(segment.c_str(), segment.size());
		}
		return hashing_t::hash_u64(relative_path.c_str(), relative_path.size());
	}

	const char* editor_panel_assets_t::get_selected_folder_path() const
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		if (!_selected_folder_node.is_null() && tree.is_valid(_selected_folder_node))
		{
			const string_t& path = tree.value(_selected_folder_node).full_path;
			if (!path.empty())
				return path.c_str();
		}

		return "";
	}

	sid_t editor_panel_assets_t::get_asset_guid(editor_asset_node_handle_t node) const
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		if (node.is_null() || !asset_tree.is_valid(node))
			return NULL_SID;

		const editor_asset_node_t& asset_node = asset_tree.value(node);
		if (asset_node.type != editor_asset_node_type_e::asset)
			return NULL_SID;

		const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
		return asset != nullptr ? asset->guid : NULL_SID;
	}

	bool editor_panel_assets_t::is_asset_favourite(sid_t guid) const
	{
		return guid != NULL_SID && std::find(_favourite_asset_guids.begin(), _favourite_asset_guids.end(), guid) != _favourite_asset_guids.end();
	}

	bool editor_panel_assets_t::is_folder_selected(u64 path_hash) const
	{
		return path_hash != 0 && std::find(_selected_folder_hashes.begin(), _selected_folder_hashes.end(), path_hash) != _selected_folder_hashes.end();
	}

	bool editor_panel_assets_t::is_asset_selected(editor_asset_node_handle_t node) const
	{
		return !node.is_null() && std::find(_selected_asset_nodes.begin(), _selected_asset_nodes.end(), node) != _selected_asset_nodes.end();
	}

	bool editor_panel_assets_t::is_folder_payload_root(editor_asset_node_handle_t node) const
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!node.is_null());
		SFG_ASSERT(tree.is_valid(node));

		for (u64 folder_hash : _selected_folder_hashes)
		{
			const folder_row_t* row = find_row_by_hash(folder_hash);
			if (row == nullptr || row->node.is_null() || row->node == node || !tree.is_valid(row->node))
				continue;

			if (tree.is_ancestor(row->node, node))
				return false;
		}
		return true;
	}

	bool editor_panel_assets_t::is_folder_payload_target_valid(const vector_t<editor_asset_node_handle_t>& nodes, editor_asset_node_handle_t target_folder_node) const
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		if (target_folder_node.is_null() || !tree.is_valid(target_folder_node))
			return false;

		const editor_asset_node_t& target_node = tree.value(target_folder_node);
		if (target_node.type != editor_asset_node_type_e::folder)
			return false;

		for (editor_asset_node_handle_t node : nodes)
		{
			if (node.is_null() || !tree.is_valid(node))
				continue;

			const editor_asset_node_t& folder_node = tree.value(node);
			if (folder_node.type != editor_asset_node_type_e::folder)
				continue;

			if (node == target_folder_node || tree.is_ancestor(node, target_folder_node))
				return false;
		}
		return true;
	}

	size_t editor_panel_assets_t::find_visible_folder_index(u64 path_hash) const
	{
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			if (_folder_rows[i].path_hash == path_hash)
				return i;
		}
		return SIZE_MAX;
	}

	size_t editor_panel_assets_t::find_visible_asset_index(editor_asset_node_handle_t node) const
	{
		for (size_t i = 0; i < _asset_grid_items.size(); ++i)
		{
			if (_asset_grid_items[i].node == node)
				return i;
		}
		return SIZE_MAX;
	}

	bool editor_panel_assets_t::is_focus_in_folder_pane() const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		ui::widget_id_t			 cur  = _ui->get_input().get_focused();
		while (cur != NULL_WIDGET && tree.is_alive(cur))
		{
			if (cur == _assets_left_pane_body)
				return true;
			cur = tree.node(cur).parent;
		}
		return false;
	}

	bool editor_panel_assets_t::is_focus_in_asset_items() const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		ui::widget_id_t			 cur  = _ui->get_input().get_focused();
		while (cur != NULL_WIDGET && tree.is_alive(cur))
		{
			if (cur == _assets_body_pane_top)
				return true;
			cur = tree.node(cur).parent;
		}
		return false;
	}

	bool editor_panel_assets_t::select_folder_by_full_path(const char* path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if (!row.node.is_null() && tree.is_valid(row.node) && tree.value(row.node).full_path == path)
			{
				select_folder_row(row.node, row.path_hash, false, false);
				return true;
			}
		}
		return false;
	}

	bool editor_panel_assets_t::select_asset_by_full_path(const char* path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			if (!item.node.is_null() && tree.is_valid(item.node) && tree.value(item.node).full_path == path)
			{
				select_asset_grid_item(item.node, false, false);
				return true;
			}
		}
		return false;
	}

	bool editor_panel_assets_t::has_favourite_asset_descendant(editor_asset_node_handle_t node) const
	{
		const editor_asset_tree_t& tree	 = editor_asset_manager_t::get().get_asset_tree();
		editor_asset_node_handle_t child = tree.first_child(node);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = tree.value(child);
			if ((child_node.flags & editor_asset_node_flag_hidden) == 0)
			{
				if (child_node.type == editor_asset_node_type_e::asset && is_asset_favourite(get_asset_guid(child)))
					return true;
				if (child_node.type == editor_asset_node_type_e::folder && has_favourite_asset_descendant(child))
					return true;
			}
			child = tree.next_sibling(child);
		}
		return false;
	}

	const editor_panel_assets_t::folder_row_t* editor_panel_assets_t::find_row_by_hash(u64 path_hash) const
	{
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if (row.path_hash == path_hash)
				return &row;
		}
		return nullptr;
	}

	const editor_panel_assets_t::folder_row_t* editor_panel_assets_t::find_row_by_widget(ui::widget_id_t id, bool match_icon) const
	{
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if ((match_icon ? row.icon : row.root) == id)
				return &row;
		}
		return nullptr;
	}

	const editor_panel_assets_t::folder_row_t* editor_panel_assets_t::find_row_by_pos(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t&		row = _folder_rows[i];
			const ui::layout_out_t& out = tree.out(row.root);
			if (rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(pos))
				return &row;
		}
		return nullptr;
	}

	const editor_panel_assets_t::asset_grid_item_t* editor_panel_assets_t::find_asset_grid_item_by_widget(ui::widget_id_t id) const
	{
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			if (item.root == id)
				return &item;
		}
		return nullptr;
	}

}
