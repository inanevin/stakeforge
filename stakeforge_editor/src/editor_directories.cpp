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
#include "editor_settings.hpp"
#include <sfg/io/file_system.hpp>

namespace sfg
{
	namespace
	{
		constexpr const char* STAKEFORGE_DIR = "stakeforge";
		constexpr const char* SETTINGS_FILE	 = "editor.sfg";
	}

	string_t editor_directories_t::get_user_directory()
	{
		string_t dir = file_system_t::get_user_directory();
		file_system_t::fix_path(dir);
		if (!dir.empty() && dir.back() != '/')
			dir += '/';
		dir += STAKEFORGE_DIR;
		dir += '/';
		return dir;
	}

	string_t editor_directories_t::get_settings_path()
	{
		return get_user_directory() + SETTINGS_FILE;
	}

	string_t editor_directories_t::get_editor_assets()
	{
		return string_t(SFG_ROOT_DIRECTORY) + "assets/";
	}

	string_t editor_directories_t::get_editor_resource_cache()
	{
		return get_user_directory() + "editor/resource_cache/";
	}

	string_t editor_directories_t::get_editor_manifest()
	{
		return string_t(SFG_ROOT_DIRECTORY) + "assets/assets_editor.sfg";
	}

	string_t editor_directories_t::get_project_assets_directory()
	{
		return get_project_assets_directory(editor_settings_t::get().get_project());
	}

	string_t editor_directories_t::get_project_assets_directory(const editor_project_t& project)
	{
		string_t project_path = file_system_t::get_absolute_path(project.path.c_str());
		if (project_path.empty())
			project_path = project.path;
		file_system_t::fix_path(project_path);

		string_t directory = file_system_t::get_directory_of_file(project_path.c_str());
		file_system_t::fix_path(directory);
		if (!directory.empty() && directory.back() != '/')
			directory += '/';
		directory += "assets/";
		return directory;
	}

	string_t editor_directories_t::get_project_asset_cache_directory(const editor_project_t& project)
	{
		return get_project_assets_directory(project) + "_cache/";
	}

	bool editor_directories_t::ensure_project_assets_directory(const editor_project_t& project)
	{
		const string_t assets_dir = get_project_assets_directory(project);
		if (!file_system_t::exists(assets_dir.c_str()) && !file_system_t::create_directory(assets_dir.c_str()))
			return false;

		const string_t cache_dir = get_project_asset_cache_directory(project);
		if (!file_system_t::exists(cache_dir.c_str()) && !file_system_t::create_directory(cache_dir.c_str()))
			return false;

		return true;
	}
}
