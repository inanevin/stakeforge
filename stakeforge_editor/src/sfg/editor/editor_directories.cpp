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
#include "editor/editor_directories.hpp"
#include "editor/editor_project.hpp"
#include <sfg/core/io/file_system.hpp>

namespace sfg
{

#define STAKEFORGE_DIR "stakeforge/";
#define SETTINGS_FILE  "editor.sfg";

	string_t editor_directories_t::s_user_directory		   = "";
	string_t editor_directories_t::s_editor_settings	   = "";
	string_t editor_directories_t::s_editor_assets		   = "";
	string_t editor_directories_t::s_editor_resource_cache = "";
	string_t editor_directories_t::s_assets_manifest	   = "";

	bool editor_directories_t::is_valid_asset_name(const char* name)
	{
		if (name == nullptr || name[0] == '\0')
			return false;

		for (const char* it = name; *it != '\0'; ++it)
		{
			const char c = *it;
			if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '\"' || c == '<' || c == '>' || c == '|')
				return false;
		}
		return true;
	}

	void editor_directories_t::init_paths()
	{
		// user dir
		s_user_directory = file_system_t::get_user_directory();
		file_system_t::fix_path(s_user_directory);
		file_system_t::fix_path_end_slash(s_user_directory);
		s_user_directory += STAKEFORGE_DIR;

		// editor settings
		s_editor_settings = s_user_directory + SETTINGS_FILE;

		// editor assets
		s_editor_assets = file_system_t::get_running_directory() + "assets/";

		// editor res cache
		s_editor_resource_cache = s_user_directory + "editor/resource_cache/";

		s_assets_manifest = s_editor_assets + "assets_manifest.sfg_asset";

		if (!file_system_t::exists(s_user_directory.c_str()))
			file_system_t::create_directory(s_user_directory.c_str());
	}
}
