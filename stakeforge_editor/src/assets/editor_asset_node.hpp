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

#include <sfg/data/string.hpp>
#include <sfg/data/tree.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	enum class editor_asset_node_type_e : u8
	{
		folder,
		file,
		script_file,
		asset,
	};

	enum editor_asset_node_flags_e : u8
	{
		editor_asset_node_flag_hidden	= 1 << 0, // folder name begins with '_' — never shown in the assets panel
		editor_asset_node_flag_promoted = 1 << 1, // root assets folder — its rows are collapsed into the parent listing
	};

	struct editor_asset_node_t
	{
		u64						 asset_id = 0;
		string_t				 name;
		string_t				 full_path;
		editor_asset_node_type_e type  = editor_asset_node_type_e::folder;
		u8						 flags = 0;
	};

	using editor_asset_node_handle_t = pool_handle_t<u32, editor_asset_node_t>;
	using editor_asset_tree_t		 = tree_t<editor_asset_node_t>;
}
