// Copyright (c) 2025 Inan Evin

#include "engine_directories.hpp"
#include <sfg/io/file_system.hpp>

namespace sfg
{
	namespace
	{
		constexpr const char* STAKEFORGE_DIR = "stakeforge";
	}

	string_t engine_directories_t::get_user_directory()
	{
		string_t dir = file_system_t::get_user_directory();
		file_system_t::fix_path(dir);
		if (!dir.empty() && dir.back() != '/')
			dir += '/';
		dir += STAKEFORGE_DIR;
		dir += '/';
		return dir;
	}

	string_t engine_directories_t::get_engine_assets()
	{
		return string_t(SFG_ROOT_DIRECTORY) + "assets/engine/";
	}

	string_t engine_directories_t::get_engine_resource_cache()
	{
		return get_user_directory() + "engine/resource_cache/";
	}

	string_t engine_directories_t::get_engine_manifest()
	{
		return string_t(SFG_ROOT_DIRECTORY) + "assets/assets_engine.sfg";
	}
}
