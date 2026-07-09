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
