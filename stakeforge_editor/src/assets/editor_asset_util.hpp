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

#include "assets/editor_asset_node.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	struct editor_asset_t;
	struct mesh_def_t;
	struct animation_def_t;

	class editor_asset_util_t final
	{
	public:
		editor_asset_util_t()									   = delete;
		~editor_asset_util_t()									   = delete;
		editor_asset_util_t(const editor_asset_util_t&)			   = delete;
		editor_asset_util_t& operator=(const editor_asset_util_t&) = delete;

		static sid_t					  generate_unique_asset_guid(span_t<const sid_t> reserved_guids = {});
		static sid_t					  try_read_existing_guid(const char* path);
		static const editor_asset_node_t* find_asset_node(sid_t guid);
		static string_t					  find_asset_path(sid_t guid);
		static const char*				  find_asset_display_name(sid_t guid);
		static bool						  load_animation_def(const editor_asset_t& asset, animation_def_t& out);
		static bool						  load_mesh_def(const editor_asset_t& asset, mesh_def_t& out);
		static bool						  duplicate_folder(editor_asset_node_handle_t folder_node, string_t* out_duplicated_path = nullptr);
		static bool						  rename_folder(editor_asset_node_handle_t folder_node, const char* new_path);
		static bool						  move_folder(editor_asset_node_handle_t folder_node, editor_asset_node_handle_t target_folder_node);
		static bool						  rename_file(editor_asset_node_handle_t file_node, const char* new_path);
		static bool						  duplicate_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, string_t* out_duplicated_path = nullptr);
		static bool						  rename_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, const char* new_path);
		static bool						  move_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, editor_asset_node_handle_t target_folder_node);
	};
}
