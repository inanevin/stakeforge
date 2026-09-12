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

#include "assets/editor_asset_importer.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_work_context_t;

	class editor_asset_manager_util_t final
	{
	public:
		editor_asset_manager_util_t()											   = delete;
		~editor_asset_manager_util_t()											   = delete;
		editor_asset_manager_util_t(const editor_asset_manager_util_t&)			   = delete;
		editor_asset_manager_util_t& operator=(const editor_asset_manager_util_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static void build_asset_database(editor_asset_manager_t& asset_manager, const char* assets_dir);
		static void ensure_integrity(editor_asset_manager_t& asset_manager, sid_t asset_id);
		static bool ensure_project_assets(editor_asset_manager_t& asset_manager, editor_work_context_t& work_context);
		static void import_assets(const char* target_directory, span_t<const string_t> paths, span_t<const editor_asset_import_options_t> import_options, editor_work_context_t& work_context, vector_t<string_t>& out_imported_asset_paths);
		static void ensure_default_meshes();
	};
}
