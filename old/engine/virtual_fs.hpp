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

#pragma once

#include "common/size_definitions.hpp"
#include "common/string_id.hpp"
#include "data/hash_map.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"

namespace sfg
{
	enum class virtual_fs_mode_t : u8
	{
		package,
		directory,
	};

	enum class package_type_t : u8
	{
		engine,
		resources,
		levels,
	};

	struct package_identifier_t
	{
		string_t	   package_name = {};
		package_type_t package_type = package_type_t::resources;
	};

	struct virtual_fs_config_t
	{
		virtual_fs_mode_t			   mode				= virtual_fs_mode_t::package;
		vector_t<string_t>			   root_directories = {};
		vector_t<package_identifier_t> packages			= {};
	};

	struct vfs_entry_t
	{
		string_t relative_path = {};
		string_t absolute_path = {};
		string_t filename	   = {};
		string_t extension	   = {};
	};

	class virtual_fs_t
	{
	public:
		bool init(const virtual_fs_config_t& config);
		void uninit();
		bool refresh();

		const string_t& get_file_abs(string_id relative_path_hash) const;

		inline bool is_directory_mode() const
		{
			return _config.mode == virtual_fs_mode_t::directory;
		}

		inline const virtual_fs_config_t& get_config() const
		{
			return _config;
		}

		inline const hash_map_t<string_id, vfs_entry_t>& get_entries() const
		{
			return _entries;
		}

	private:
		bool validate_directory_mode_config();
		bool validate_package_mode_config() const;
		void refresh_directory_entries();

	private:
		virtual_fs_config_t				   _config	= {};
		hash_map_t<string_id, vfs_entry_t> _entries = {};
	};
}
