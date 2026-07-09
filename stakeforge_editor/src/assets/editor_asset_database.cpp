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

#include "assets/editor_asset_database.hpp"
#include "assets/editor_asset_path.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <utility>

namespace sfg
{
	void editor_asset_database_t::clear()
	{
		_asset_tree.clear();
		_assets.clear();
		_asset_nodes.clear();
		_path_nodes.clear();
		_root_node = {};
	}

	void editor_asset_database_t::reserve(u32 node_capacity, u32 asset_capacity)
	{
		_asset_tree.reserve(node_capacity);
		_assets.reserve(asset_capacity);
		_asset_nodes.reserve(asset_capacity);
		_path_nodes.reserve(node_capacity);
	}

	void editor_asset_database_t::rebuild_indices()
	{
		_asset_nodes.clear();
		_path_nodes.clear();
		_asset_nodes.reserve(_assets.size());
		_path_nodes.reserve(_asset_tree.size());
		for (auto it = _asset_tree.begin_handle(); it != _asset_tree.end_handle(); ++it)
			index_node(*it);
	}

	editor_asset_node_handle_t editor_asset_database_t::emplace_node(editor_asset_node_t&& node)
	{
		const editor_asset_node_handle_t handle = _asset_tree.emplace(std::move(node));
		index_node(handle);
		return handle;
	}

	void editor_asset_database_t::attach_node(editor_asset_node_handle_t parent, editor_asset_node_handle_t child)
	{
		_asset_tree.attach(parent, child);
	}

	void editor_asset_database_t::remove_node_subtree(editor_asset_node_handle_t node)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		_asset_tree.for_each_depth_first(node, [&](editor_asset_node_handle_t current, u32) {
			const editor_asset_node_t& value = _asset_tree.value(current);
			if (value.type == editor_asset_node_type_e::asset)
				_assets.erase(value.asset_id);
			unindex_node(current);
		});
		if (node == _root_node)
			_root_node = {};
		_asset_tree.remove_subtree(node);
	}

	void editor_asset_database_t::set_root_node(editor_asset_node_handle_t node)
	{
		SFG_ASSERT(node.is_null() || _asset_tree.is_valid(node));
		_root_node = node;
	}

	void editor_asset_database_t::upsert_asset(editor_asset_t&& asset)
	{
		SFG_ASSERT(asset.guid != NULL_SID);
		_assets[asset.guid] = std::move(asset);
	}

	void editor_asset_database_t::erase_asset(sid_t guid)
	{
		_assets.erase(guid);
		_asset_nodes.erase(guid);
	}

	void editor_asset_database_t::update_node_path(editor_asset_node_handle_t node, const char* new_path)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		SFG_ASSERT(new_path != nullptr);
		SFG_ASSERT(new_path[0] != '\0');

		editor_asset_node_t& value	  = _asset_tree.value(node);
		const string_t		 old_path = value.full_path;
		unindex_node(node);
		value.full_path = new_path;
		if (value.type == editor_asset_node_type_e::folder)
			value.name = file_system_t::get_last_folder_from_path(new_path);
		else if (value.type == editor_asset_node_type_e::asset)
			value.name = file_system_t::remove_extensions_from_path(file_system_t::get_filename_and_extension_from_path(new_path));
		else
			value.name = file_system_t::get_filename_and_extension_from_path(new_path);
		index_node(node);

		if (value.type == editor_asset_node_type_e::folder)
			update_descendant_paths(node, old_path, value.full_path);
	}

	void editor_asset_database_t::move_node(editor_asset_node_handle_t node, editor_asset_node_handle_t new_parent, const char* new_path)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		SFG_ASSERT(_asset_tree.is_valid(new_parent));
		_asset_tree.attach(new_parent, node);
		update_node_path(node, new_path);
	}

	editor_asset_t* editor_asset_database_t::find_asset(sid_t guid)
	{
		const auto it = _assets.find(guid);
		return it != _assets.end() ? &it->second : nullptr;
	}

	const editor_asset_t* editor_asset_database_t::find_asset(sid_t guid) const
	{
		const auto it = _assets.find(guid);
		return it != _assets.end() ? &it->second : nullptr;
	}

	editor_asset_node_handle_t editor_asset_database_t::find_asset_node(sid_t guid) const
	{
		const auto it = _asset_nodes.find(guid);
		return it != _asset_nodes.end() ? it->second : editor_asset_node_handle_t{};
	}

	editor_asset_node_handle_t editor_asset_database_t::find_node_by_path(const char* path) const
	{
		const auto it = _path_nodes.find(editor_asset_path_t::hash_path(path));
		return it != _path_nodes.end() ? it->second : editor_asset_node_handle_t{};
	}

	const editor_asset_node_t* editor_asset_database_t::find_asset_node_value(sid_t guid) const
	{
		const editor_asset_node_handle_t node = find_asset_node(guid);
		return !node.is_null() && _asset_tree.is_valid(node) ? &_asset_tree.value(node) : nullptr;
	}

	const char* editor_asset_database_t::find_asset_display_name(sid_t guid) const
	{
		const editor_asset_node_t* node = find_asset_node_value(guid);
		return node != nullptr ? node->name.c_str() : nullptr;
	}

	string_t editor_asset_database_t::find_asset_path(sid_t guid) const
	{
		const editor_asset_node_t* node = find_asset_node_value(guid);
		return node != nullptr ? node->full_path : string_t{};
	}

	void editor_asset_database_t::index_node(editor_asset_node_handle_t node)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		const editor_asset_node_t& value = _asset_tree.value(node);
		if (!value.full_path.empty())
			_path_nodes[editor_asset_path_t::hash_path(value.full_path.c_str())] = node;
		if (value.type == editor_asset_node_type_e::asset)
			_asset_nodes[value.asset_id] = node;
	}

	void editor_asset_database_t::unindex_node(editor_asset_node_handle_t node)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		const editor_asset_node_t& value = _asset_tree.value(node);
		if (!value.full_path.empty())
			_path_nodes.erase(editor_asset_path_t::hash_path(value.full_path.c_str()));
		if (value.type == editor_asset_node_type_e::asset)
			_asset_nodes.erase(value.asset_id);
	}

	void editor_asset_database_t::update_descendant_paths(editor_asset_node_handle_t node, const string_t& old_prefix, const string_t& new_prefix)
	{
		SFG_ASSERT(_asset_tree.is_valid(node));
		string_t old_dir = editor_asset_path_t::normalize_directory(old_prefix.c_str());
		string_t new_dir = editor_asset_path_t::normalize_directory(new_prefix.c_str());
		_asset_tree.for_each_depth_first(node, [&](editor_asset_node_handle_t current, u32) {
			if (current == node)
				return;

			editor_asset_node_t& value = _asset_tree.value(current);
			unindex_node(current);
			value.full_path = new_dir + value.full_path.substr(old_dir.size());
			index_node(current);
		});
	}
}
