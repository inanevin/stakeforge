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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_path_t final
	{
	public:
		editor_asset_path_t()									   = delete;
		~editor_asset_path_t()									   = delete;
		editor_asset_path_t(const editor_asset_path_t&)			   = delete;
		editor_asset_path_t& operator=(const editor_asset_path_t&) = delete;

		static string_t normalize_directory(const char* directory);
		static string_t make_asset_path(const char* directory, const char* asset_name);
		static string_t make_blob_path(const char* directory, const char* asset_name);
		static string_t make_source_path(const char* directory, const char* file_name, const char* extension);
		static string_t make_unique_source_path(const char* directory, const char* file_name, const char* extension);
		static string_t get_cache_path_for_guid(sid_t guid);
		static string_t get_source_full_path(const char* assets_path, const editor_asset_t& asset);
		static string_t get_source_relative(const char* assets_path, const char* source_full_path);
		static bool		set_source_relative_or_copy(editor_asset_t& asset, const char* asset_directory, const char* asset_name, const char* source_full_path);
		static bool		is_source_inside_assets(const char* assets_path, const char* source_full_path);
		static bool		is_same_path(const char* lhs, const char* rhs);
		static bool		is_path_in_directory(const char* path, const char* directory);
		static u64		hash_path(const char* path);
	};
}
