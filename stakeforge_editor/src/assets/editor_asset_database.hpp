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

#pragma once

#include "assets/editor_asset.hpp"

#include <sfg/data/hash_map.hpp>

namespace sfg
{
	class editor_asset_database_t final
	{
	public:
		editor_asset_database_t()										   = default;
		~editor_asset_database_t()										   = default;
		editor_asset_database_t(const editor_asset_database_t&)			   = delete;
		editor_asset_database_t& operator=(const editor_asset_database_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void clear();
		void reserve(u32 node_capacity, u32 asset_capacity);
		void rebuild_indices();

		// -----------------------------------------------------------------------------
		// mutation
		// -----------------------------------------------------------------------------

		editor_asset_node_handle_t emplace_node(editor_asset_node_t&& node);
		void					   attach_node(editor_asset_node_handle_t parent, editor_asset_node_handle_t child);
		void					   remove_node_subtree(editor_asset_node_handle_t node);
		void					   set_root_node(editor_asset_node_handle_t node);
		void					   upsert_asset(editor_asset_t&& asset);
		void					   erase_asset(sid_t guid);
		void					   update_node_path(editor_asset_node_handle_t node, const char* new_path);
		void					   move_node(editor_asset_node_handle_t node, editor_asset_node_handle_t new_parent, const char* new_path);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		editor_asset_t*			   find_asset(sid_t guid);
		const editor_asset_t*	   find_asset(sid_t guid) const;
		editor_asset_node_handle_t find_asset_node(sid_t guid) const;
		editor_asset_node_handle_t find_node_by_path(const char* path) const;
		const editor_asset_node_t* find_asset_node_value(sid_t guid) const;
		const char*				   find_asset_display_name(sid_t guid) const;
		string_t				   find_asset_path(sid_t guid) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_asset_tree_t& get_asset_tree()
		{
			return _asset_tree;
		}

		inline const editor_asset_tree_t& get_asset_tree() const
		{
			return _asset_tree;
		}

		inline hash_map_t<u64, editor_asset_t>& get_assets()
		{
			return _assets;
		}

		inline const hash_map_t<u64, editor_asset_t>& get_assets() const
		{
			return _assets;
		}

		inline editor_asset_node_handle_t get_root_node() const
		{
			return _root_node;
		}

	private:
		void index_node(editor_asset_node_handle_t node);
		void unindex_node(editor_asset_node_handle_t node);
		void update_descendant_paths(editor_asset_node_handle_t node, const string_t& old_prefix, const string_t& new_prefix);

	private:
		editor_asset_tree_t							  _asset_tree;
		hash_map_t<u64, editor_asset_t>				  _assets;
		hash_map_t<sid_t, editor_asset_node_handle_t> _asset_nodes;
		hash_map_t<u64, editor_asset_node_handle_t>	  _path_nodes;
		editor_asset_node_handle_t					  _root_node;
	};
}
