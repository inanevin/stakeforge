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

#include "virtual_fs.hpp"

#include "file_types.hpp"
#include "data/string_util.hpp"
#include "io/assert.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"

namespace sfg
{
	namespace
	{
		string_t join_path(const string_t& directory, const string_t& name)
		{
			string_t path = directory;
			if (!path.empty() && path.back() != '/')
				path += '/';
			path += name;
			file_system::fix_path(path);
			return path;
		}
	}

	bool virtual_fs_t::init(const virtual_fs_config_t& config)
	{
		_config = config;
		if (!refresh())
		{
			_entries.clear();
			return false;
		}

		return true;
	}

	void virtual_fs_t::uninit()
	{
		_entries.clear();
		_config = {};
	}

	bool virtual_fs_t::refresh()
	{
		_entries.clear();

		if (_config.mode == virtual_fs_mode_t::directory)
		{
			if (!validate_directory_mode_config())
				return false;

			refresh_directory_entries();
			return true;
		}

		return validate_package_mode_config();
	}

	const string_t& virtual_fs_t::get_file_abs(string_id relative_path_hash) const
	{
		SFG_ASSERT(_config.mode == virtual_fs_mode_t::directory);

		const auto it = _entries.find(relative_path_hash);
		SFG_ASSERT(it != _entries.end());
		return it->second.absolute_path;
	}

	bool virtual_fs_t::validate_directory_mode_config()
	{
		for (string_t& root_directory : _config.root_directories)
		{
			if (root_directory.empty())
			{
				SFG_ERR("virtual_fs directory mode received an empty root directory.");
				return false;
			}

			if (!file_system::is_absolute_path(root_directory.c_str()))
			{
				SFG_ERR("virtual_fs directory mode requires absolute root directories: {0}", root_directory);
				return false;
			}

			if (!file_system::exists(root_directory.c_str()))
			{
				SFG_ERR("virtual_fs root directory does not exist: {0}", root_directory);
				return false;
			}

			if (!file_system::is_directory(root_directory.c_str()))
			{
				SFG_ERR("virtual_fs root directory is not a directory: {0}", root_directory);
				return false;
			}

			root_directory = file_system::get_absolute_path(root_directory.c_str());
			if (root_directory.empty())
			{
				SFG_ERR("virtual_fs failed resolving absolute root directory path.");
				return false;
			}
		}

		return true;
	}

	bool virtual_fs_t::validate_package_mode_config() const
	{
		const string_t running_directory = file_system::get_running_directory();
		if (running_directory.empty())
		{
			SFG_ERR("virtual_fs failed to resolve running directory.");
			return false;
		}

		for (const package_identifier_t& package : _config.packages)
		{
			if (package.package_name.empty())
			{
				SFG_ERR("virtual_fs package mode received an empty package name.");
				return false;
			}

			const string_t package_path = join_path(running_directory, package.package_name);
			if (!file_system::exists(package_path.c_str()))
			{
				SFG_ERR("virtual_fs package does not exist in running directory: {0}", package.package_name);
				return false;
			}
		}

		return true;
	}

	void virtual_fs_t::refresh_directory_entries()
	{
		for (const string_t& root_directory : _config.root_directories)
		{
			vector_t<string_t> files = {};
			file_system::get_files_recursive(root_directory.c_str(), files);

			for (const string_t& absolute_path : files)
			{
				string_t extension = file_system::get_file_extension(absolute_path);
				string_util::to_lower(extension);
				if (g_file_types.find(extension) == g_file_types.end())
					continue;

				const string_t relative_path = file_system::get_relative(root_directory.c_str(), absolute_path.c_str());

				vfs_entry_t entry	= {};
				entry.relative_path = relative_path;
				entry.absolute_path = absolute_path;
				entry.filename		= file_system::get_filename_from_path(absolute_path);
				entry.extension		= extension;

				const string_id relative_path_hash = to_sid(entry.relative_path);
				const auto		insert_result	   = _entries.try_emplace(relative_path_hash, std::move(entry));
				if (!insert_result.second)
					SFG_WARN("virtual_fs duplicate relative path ignored: {0}", relative_path);
			}
		}
	}
}
