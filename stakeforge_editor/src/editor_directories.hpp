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

namespace sfg
{
	struct editor_project_t;

	class editor_directories_t
	{
	public:
		static inline const string_t& get_editor_resource_cache()
		{
			return s_editor_resource_cache;
		}

		static inline const string_t& get_editor_manifest()
		{
			return s_editor_manifest;
		}

		static inline const string_t& get_engine_manifest()
		{
			return s_engine_manifest;
		}

		static inline const string_t& get_user_directory()
		{
			return s_user_directory;
		}

		static inline const string_t& get_editor_assets()
		{
			return s_editor_assets;
		}

		static inline const string_t& get_editor_settings()
		{
			return s_editor_settings;
		}

		static bool is_valid_asset_name(const char* name);
		static bool is_valid_csharp_identifier(const char* name);

	private:
		friend class editor_app_t;

		static string_t s_user_directory;
		static string_t s_editor_settings;
		static string_t s_editor_assets;
		static string_t s_editor_resource_cache;
		static string_t s_editor_manifest;
		static string_t s_engine_manifest;

		static void init_paths();
	};
}
