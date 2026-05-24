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
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include <sfg/io/file_system.hpp>

namespace sfg
{

#define STAKEFORGE_DIR "stakeforge/";
#define SETTINGS_FILE  "editor.sfg";

	string_t editor_directories_t::s_user_directory		   = "";
	string_t editor_directories_t::s_editor_settings	   = "";
	string_t editor_directories_t::s_editor_assets		   = "";
	string_t editor_directories_t::s_editor_resource_cache = "";
	string_t editor_directories_t::s_editor_manifest	   = "";

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
		s_editor_assets = string_t(SFG_ROOT_DIRECTORY) + "assets/";

		// editor res cache
		s_editor_resource_cache = s_user_directory + "editor/resource_cache/";

		// manifest
		s_editor_manifest = string_t(SFG_ROOT_DIRECTORY) + "assets/assets_editor.sfg";
	}
}
