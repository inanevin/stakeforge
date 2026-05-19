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

#include "assets/editor_asset_node.hpp"

#include <sfg/data/string.hpp>
#include <sfg/data/tree.hpp>

namespace sfg
{
	struct editor_project_t;

	using editor_asset_tree_t		 = tree_t<editor_asset_node_t>;
	using editor_asset_node_handle_t = editor_asset_tree_t::handle_t;

	class editor_asset_manager_t final
	{
	public:
		editor_asset_manager_t()										 = default;
		~editor_asset_manager_t()										 = default;
		editor_asset_manager_t(const editor_asset_manager_t&)			 = delete;
		editor_asset_manager_t& operator=(const editor_asset_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const editor_project_t& project);
		void uninit();
		void clear();
		bool rescan(const editor_project_t& project);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const editor_asset_tree_t& get_asset_tree() const
		{
			return _asset_tree;
		}

		inline editor_asset_node_handle_t get_root_node() const
		{
			return _root_node;
		}

		inline u32 get_generation() const
		{
			return _generation;
		}

		static editor_asset_manager_t& get();

	private:
		bool					   build_asset_tree(const string_t& assets_dir);
		bool					   read_asset(const char* path, editor_asset_t& out_asset) const;
		editor_asset_node_handle_t find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const;
		editor_asset_node_handle_t get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name);

	private:
		editor_asset_tree_t		   _asset_tree;
		editor_asset_node_handle_t _root_node;
		u32						   _generation = 0;
	};
}
