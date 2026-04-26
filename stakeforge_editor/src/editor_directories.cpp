// Copyright (c) 2025 Inan Evin

#include "editor_directories.hpp"

#include "io/file_system.hpp"

namespace sfg
{
	namespace
	{
		constexpr const char* STAKEFORGE_DIR = "stakeforge";
		constexpr const char* SETTINGS_FILE	 = "editor.sfg";
	}

	string_t editor_directories_t::get_user_directory()
	{
		string_t dir = file_system::get_user_directory();
		file_system::fix_path(dir);
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
}
