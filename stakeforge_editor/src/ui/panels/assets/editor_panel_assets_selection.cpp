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
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_surface_controller.hpp"
#include "ui/panels/assets/editor_panel_assets_internal.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include "ui/editor_payload_controller.hpp"
#include "assets/editor_asset_manager.hpp"
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_assets_t::set_focus_state(bool focused)
	{
		if (_focused == focused)
			return;

		_focused = focused;
		for (const folder_row_t& row : _folder_rows)
			update_folder_row_background(row);
		refresh_asset_grid_item_backgrounds();
	}

	bool editor_panel_assets_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_panel_assets_t::request_ui_mutation()
	{
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_assets_t::flush_pending_ui_mutations()
	{
		if (_pending_show_asset_guid != NULL_SID)
		{
			const sid_t guid		 = _pending_show_asset_guid;
			_pending_show_asset_guid = NULL_SID;
			show_asset(guid);
			return;
		}

		const bool refresh_folders = _folder_rows_refresh_pending;
		const bool refresh_assets  = _asset_grid_refresh_pending;
		const bool force_assets	   = _asset_grid_refresh_force;

		_folder_rows_refresh_pending = false;
		_asset_grid_refresh_pending	 = false;
		_asset_grid_refresh_force	 = false;

		if (refresh_folders)
		{
			refresh_folder_rows();
			return;
		}

		if (refresh_assets)
			refresh_asset_grid(force_assets);
	}

	void editor_panel_assets_t::notify_asset_selection_changed()
	{
		if (editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::inspector))
			static_cast<editor_panel_inspector_t*>(panel)->on_asset_selection_changed();
	}

	void editor_panel_assets_t::select_folder_row(editor_asset_node_handle_t node, u64 path_hash, bool range_select, bool incremental_select)
	{
		clear_asset_grid_selection();

		if (range_select && _folder_selection_anchor != 0)
		{
			if (!incremental_select)
				_selected_folder_hashes.resize(0);

			const size_t anchor_index = find_visible_folder_index(_folder_selection_anchor);
			const size_t folder_index = find_visible_folder_index(path_hash);
			if (anchor_index != SIZE_MAX && folder_index != SIZE_MAX)
			{
				const size_t first = std::min(anchor_index, folder_index);
				const size_t last  = std::max(anchor_index, folder_index);
				for (size_t i = first; i <= last; ++i)
				{
					const u64 hash = _folder_rows[i].path_hash;
					if (hash != 0 && std::find(_selected_folder_hashes.begin(), _selected_folder_hashes.end(), hash) == _selected_folder_hashes.end())
						_selected_folder_hashes.push_back(hash);
				}
			}
			else
			{
				_selected_folder_hashes.resize(0);
				if (path_hash != 0)
					_selected_folder_hashes.push_back(path_hash);
			}
		}
		else if (incremental_select)
		{
			auto it = std::find(_selected_folder_hashes.begin(), _selected_folder_hashes.end(), path_hash);
			if (it != _selected_folder_hashes.end())
				_selected_folder_hashes.erase(it);
			else if (path_hash != 0)
				_selected_folder_hashes.push_back(path_hash);
			_folder_selection_anchor = path_hash;
		}
		else
		{
			_selected_folder_hashes.resize(0);
			if (path_hash != 0)
				_selected_folder_hashes.push_back(path_hash);
			_folder_selection_anchor = path_hash;
		}

		_selected_folder_hash			 = _selected_folder_hashes.empty() ? 0 : _selected_folder_hashes.back();
		const folder_row_t* selected_row = find_row_by_hash(_selected_folder_hash);
		_selected_folder_node			 = selected_row != nullptr ? selected_row->node : node;

		for (const folder_row_t& row : _folder_rows)
			update_folder_row_background(row);

		update_current_directory_label();
		update_import_button_state();
		refresh_asset_grid(true);
	}

	void editor_panel_assets_t::select_asset_grid_item(editor_asset_node_handle_t node, bool range_select, bool incremental_select)
	{
		if (range_select && !_asset_selection_anchor.is_null())
		{
			if (!incremental_select)
				_selected_asset_nodes.resize(0);

			const size_t anchor_index = find_visible_asset_index(_asset_selection_anchor);
			const size_t asset_index  = find_visible_asset_index(node);
			if (anchor_index != SIZE_MAX && asset_index != SIZE_MAX)
			{
				const size_t first = std::min(anchor_index, asset_index);
				const size_t last  = std::max(anchor_index, asset_index);
				for (size_t i = first; i <= last; ++i)
				{
					const editor_asset_node_handle_t item_node = _asset_grid_items[i].node;
					if (std::find(_selected_asset_nodes.begin(), _selected_asset_nodes.end(), item_node) == _selected_asset_nodes.end())
						_selected_asset_nodes.push_back(item_node);
				}
			}
			else
			{
				_selected_asset_nodes.resize(0);
				if (!node.is_null())
					_selected_asset_nodes.push_back(node);
			}
		}
		else if (incremental_select)
		{
			auto it = std::find(_selected_asset_nodes.begin(), _selected_asset_nodes.end(), node);
			if (it != _selected_asset_nodes.end())
				_selected_asset_nodes.erase(it);
			else if (!node.is_null())
				_selected_asset_nodes.push_back(node);
			_asset_selection_anchor = node;
		}
		else
		{
			_selected_asset_nodes.resize(0);
			if (!node.is_null())
				_selected_asset_nodes.push_back(node);
			_asset_selection_anchor = node;
		}

		_selected_asset_node = _selected_asset_nodes.empty() ? editor_asset_node_handle_t{} : _selected_asset_nodes.back();
		refresh_asset_grid_item_backgrounds();
		notify_asset_selection_changed();
	}

	void editor_panel_assets_t::clear_folder_selection()
	{
		_selected_folder_hash = 0;
		_selected_folder_node = {};
		_selected_folder_hashes.resize(0);
		_folder_selection_anchor = 0;
		for (const folder_row_t& row : _folder_rows)
			update_folder_row_background(row);
		update_current_directory_label();
		update_import_button_state();
	}

	void editor_panel_assets_t::collect_payload_folder_nodes(editor_asset_node_handle_t node)
	{
		_payload_folder_nodes.resize(0);

		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  tree			= asset_manager.get_asset_tree();
		bool						  node_selected = false;
		for (u64 folder_hash : _selected_folder_hashes)
		{
			const folder_row_t* row = find_row_by_hash(folder_hash);
			if (row != nullptr && row->node == node)
			{
				node_selected = true;
				break;
			}
		}

		if (node_selected)
		{
			for (u64 folder_hash : _selected_folder_hashes)
			{
				const folder_row_t* row = find_row_by_hash(folder_hash);
				if (row == nullptr || row->node.is_null() || !tree.is_valid(row->node))
					continue;

				const editor_asset_node_t& selected_node = tree.value(row->node);
				if (selected_node.type != editor_asset_node_type_e::folder || row->node == asset_manager.get_root_node() || (selected_node.flags & editor_asset_node_flag_promoted) != 0)
					continue;

				if (is_folder_payload_root(row->node))
					_payload_folder_nodes.push_back(row->node);
			}
		}

		if (_payload_folder_nodes.empty() && !node.is_null() && tree.is_valid(node))
		{
			const editor_asset_node_t& selected_node = tree.value(node);
			if (selected_node.type == editor_asset_node_type_e::folder && node != asset_manager.get_root_node() && (selected_node.flags & editor_asset_node_flag_promoted) == 0)
				_payload_folder_nodes.push_back(node);
		}
	}

	void editor_panel_assets_t::collect_payload_asset_nodes(editor_asset_node_handle_t node)
	{
		_payload_asset_nodes.resize(0);

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		if (is_asset_selected(node))
		{
			for (editor_asset_node_handle_t selected_node : _selected_asset_nodes)
			{
				if (selected_node.is_null() || !tree.is_valid(selected_node))
					continue;

				if (tree.value(selected_node).type == editor_asset_node_type_e::asset)
					_payload_asset_nodes.push_back(selected_node);
			}
		}

		if (_payload_asset_nodes.empty() && !node.is_null() && tree.is_valid(node) && tree.value(node).type == editor_asset_node_type_e::asset)
			_payload_asset_nodes.push_back(node);
	}

	bool editor_panel_assets_t::move_payload_folders(const vector_t<editor_asset_node_handle_t>& nodes, editor_asset_node_handle_t target_folder_node)
	{
		if (nodes.empty() || !is_folder_payload_target_valid(nodes, target_folder_node))
			return false;

		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		bool					   moved		 = false;
		for (editor_asset_node_handle_t node : nodes)
		{
			if (node.is_null() || !tree.is_valid(node))
				continue;

			const editor_asset_node_t& folder_node = tree.value(node);
			if (folder_node.type != editor_asset_node_type_e::folder)
				continue;

			const editor_asset_node_t& target_node = tree.value(target_folder_node);
			const string_t			   new_path	   = editor_asset_path_t::normalize_directory(target_node.full_path.c_str()) + folder_node.name;
			if (editor_asset_util_t::move_folder(node, target_folder_node))
			{
				asset_manager.move_node(node, target_folder_node, new_path.c_str());
				moved = true;
			}
		}
		return moved;
	}

	bool editor_panel_assets_t::move_payload_assets(const vector_t<editor_asset_node_handle_t>& nodes, editor_asset_node_handle_t target_folder_node, bool allow_overwrite)
	{
		if (nodes.empty())
			return false;

		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		SFG_ASSERT(!target_folder_node.is_null());
		SFG_ASSERT(tree.is_valid(target_folder_node));
		const editor_asset_node_t& target_node = tree.value(target_folder_node);
		SFG_ASSERT(target_node.type == editor_asset_node_type_e::folder);
		const string_t target_directory = editor_asset_path_t::normalize_directory(target_node.full_path.c_str());

		if (!allow_overwrite)
		{
			vector_t<string_t> rows;
			for (editor_asset_node_handle_t node : nodes)
			{
				if (node.is_null() || !tree.is_valid(node))
					continue;

				const editor_asset_node_t& asset_node = tree.value(node);
				if (asset_node.type != editor_asset_node_type_e::asset)
					continue;

				const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
				if (asset == nullptr)
					continue;

				const string_t					 file_name = file_system_t::get_filename_and_extension_from_path(asset_node.full_path);
				const string_t					 new_path  = target_directory + file_name;
				const editor_asset_node_handle_t existing  = asset_manager.find_node_by_path(new_path.c_str());
				if (existing == node)
					continue;

				string_t row;
				if (find_matching_asset_override(new_path.c_str(), asset->asset_type, &row))
					rows.push_back(row);
			}

			if (!rows.empty())
			{
				_pending_override_target_folder = target_folder_node;
				_pending_override_asset_nodes.resize(0);
				_pending_override_asset_nodes.reserve(nodes.size());
				for (editor_asset_node_handle_t node : nodes)
					_pending_override_asset_nodes.push_back(node);
				request_assets_override(asset_override_operation_e::move_assets, "One or more assets already exist in the target folder. Overwrite matching asset types?", rows);
				return true;
			}
		}

		bool moved = false;
		for (editor_asset_node_handle_t node : nodes)
		{
			if (node.is_null() || !tree.is_valid(node))
				continue;

			const editor_asset_node_t& asset_node = tree.value(node);
			if (asset_node.type != editor_asset_node_type_e::asset)
				continue;

			const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
			if (asset == nullptr)
				continue;

			const string_t					 file_name	 = file_system_t::get_filename_and_extension_from_path(asset_node.full_path);
			const string_t					 new_path	 = target_directory + file_name;
			const editor_asset_t			 asset_value = *asset;
			const editor_asset_node_handle_t existing	 = asset_manager.find_node_by_path(new_path.c_str());
			if (allow_overwrite && !existing.is_null() && existing != node && tree.is_valid(existing))
			{
				const editor_asset_node_t& existing_node = tree.value(existing);
				if (existing_node.type == editor_asset_node_type_e::asset)
				{
					const editor_asset_t* existing_asset = asset_manager.find_asset(existing_node.asset_id);
					if (existing_asset != nullptr && existing_asset->asset_type == asset_value.asset_type && editor_asset_util_t::delete_asset(*existing_asset, existing))
						asset_manager.remove_node_subtree(existing);
				}
			}

			if (editor_asset_util_t::move_asset(asset_value, node, target_folder_node))
			{
				asset_manager.move_node(node, target_folder_node, new_path.c_str());
				asset_manager.reload_asset_node(node);
				moved = true;
			}
		}
		return moved;
	}

	void editor_panel_assets_t::start_folder_payload(editor_asset_node_handle_t node)
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  tree			= asset_manager.get_asset_tree();
		SFG_ASSERT(!node.is_null());
		SFG_ASSERT(tree.is_valid(node));

		const editor_asset_node_t& folder_node = tree.value(node);
		SFG_ASSERT(folder_node.type == editor_asset_node_type_e::folder);
		if (node == asset_manager.get_root_node() || (folder_node.flags & editor_asset_node_flag_promoted) != 0)
			return;

		editor_payload_controller_t& payload_controller = editor_payload_controller_t::get();
		if (payload_controller.is_payload_active())
			return;

		collect_payload_folder_nodes(node);
		if (_payload_folder_nodes.empty())
			return;

		if (_payload_folder_nodes.size() == 1)
		{
			_payload_folder_node					= _payload_folder_nodes.front();
			const editor_asset_node_t& payload_node = tree.value(_payload_folder_node);
			payload_controller.create_payload(payload_node.name.c_str(), editor_payload_type_e::folder, &_payload_folder_node);
			return;
		}

		string_t text = std::to_string(_payload_folder_nodes.size());
		text += " folders";
		payload_controller.create_payload(text.c_str(), editor_payload_type_e::folder_multi, &_payload_folder_nodes);
	}

	void editor_panel_assets_t::start_asset_item_payload(editor_asset_node_handle_t node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!node.is_null());
		SFG_ASSERT(tree.is_valid(node));

		const editor_asset_node_t& asset_node = tree.value(node);
		if (asset_node.type != editor_asset_node_type_e::asset)
			return;

		editor_payload_controller_t& payload_controller = editor_payload_controller_t::get();
		if (payload_controller.is_payload_active())
			return;

		collect_payload_asset_nodes(node);
		if (_payload_asset_nodes.empty())
			return;

		if (_payload_asset_nodes.size() == 1)
		{
			_payload_asset_node						= _payload_asset_nodes.front();
			const editor_asset_node_t& payload_node = tree.value(_payload_asset_node);
			payload_controller.create_payload(payload_node.name.c_str(), editor_payload_type_e::asset, &_payload_asset_node);
			return;
		}

		string_t text = std::to_string(_payload_asset_nodes.size());
		text += " assets";
		payload_controller.create_payload(text.c_str(), editor_payload_type_e::asset_multi, &_payload_asset_nodes);
	}

	void editor_panel_assets_t::clear_asset_grid_selection()
	{
		if (_selected_asset_node.is_null() && _selected_asset_nodes.empty())
			return;

		_selected_asset_node = {};
		_selected_asset_nodes.resize(0);
		_asset_selection_anchor = {};
		refresh_asset_grid_item_backgrounds();
		notify_asset_selection_changed();
	}

	void editor_panel_assets_t::select_all_visible_folders()
	{
		clear_asset_grid_selection();
		_selected_folder_hashes.resize(0);
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if (row.path_hash != 0)
				_selected_folder_hashes.push_back(row.path_hash);
		}
		_selected_folder_hash	 = _selected_folder_hashes.empty() ? 0 : _selected_folder_hashes.back();
		_folder_selection_anchor = _selected_folder_hash;
		const folder_row_t* row	 = find_row_by_hash(_selected_folder_hash);
		_selected_folder_node	 = row != nullptr ? row->node : editor_asset_node_handle_t{};
		for (const folder_row_t& folder_row : _folder_rows)
			update_folder_row_background(folder_row);
		update_current_directory_label();
		update_import_button_state();
		refresh_asset_grid(true);
	}

	void editor_panel_assets_t::select_all_visible_assets()
	{
		_selected_asset_nodes.resize(0);
		for (const asset_grid_item_t& item : _asset_grid_items)
			_selected_asset_nodes.push_back(item.node);
		_selected_asset_node	= _selected_asset_nodes.empty() ? editor_asset_node_handle_t{} : _selected_asset_nodes.back();
		_asset_selection_anchor = _selected_asset_node;
		refresh_asset_grid_item_backgrounds();
		notify_asset_selection_changed();
	}

	void editor_panel_assets_t::toggle_folder_fold(u64 path_hash)
	{
		auto it = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), path_hash);
		if (it != _expanded_folder_hashes.end())
			_expanded_folder_hashes.erase(it);
		else
			_expanded_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_folder_favourite(u64 path_hash)
	{
		auto it = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), path_hash);
		if (it != _favourite_folder_hashes.end())
			_favourite_folder_hashes.erase(it);
		else
			_favourite_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_asset_favourite(sid_t guid)
	{
		auto it = std::find(_favourite_asset_guids.begin(), _favourite_asset_guids.end(), guid);
		if (it != _favourite_asset_guids.end())
			_favourite_asset_guids.erase(it);
		else
			_favourite_asset_guids.push_back(guid);
		if (_asset_favourites_only)
			refresh_asset_grid(true);
		refresh_asset_favourite_icons();
	}
}
