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

#include "resource_file_system.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>
#include <filesystem>
#include <limits>

namespace sfg
{
	void resource_file_system_t::set_mode_directory(const char* directory_path, const char* engine_cache)
	{
		SFG_ASSERT(directory_path != nullptr);
		SFG_ASSERT(engine_cache != nullptr);

		_directory_path = directory_path;
		file_system_t::fix_path(_directory_path);
		file_system_t::fix_path_end_slash(_directory_path);
		_engine_cache = engine_cache;
		file_system_t::fix_path(_engine_cache);
		file_system_t::fix_path_end_slash(_engine_cache);
		_file_pack_path.clear();
		_resource_map.clear();
		_mode = mode_e::directory;
	}

	void resource_file_system_t::set_mode_filepack(const char* path_to_file_pack, const hash_map_t<u64, resource_map_info_t>& resource_map)
	{
		SFG_ASSERT(path_to_file_pack != nullptr);

		_file_pack_path = path_to_file_pack;
		file_system_t::fix_path(_file_pack_path);
		_directory_path.clear();
		_engine_cache.clear();
		_resource_map = resource_map;
		_mode		  = mode_e::filepack;
	}

	bool resource_file_system_t::read_resource(u64 hash, size_t offset, size_t size, ostream_t& out)
	{
		if (_mode == mode_e::directory)
		{
			const string_t filename_base	  = std::to_string(hash);
			const string_t resource_filename  = filename_base + ".sfg_bin";
			const string_t thumbnail_filename = filename_base + ".sfg_thumb_bin";
			if (!_engine_cache.empty())
			{
				string_t path = _engine_cache + resource_filename;
				if (file_system_t::exists(path.c_str()))
					return read_file_range(path.c_str(), offset, size, out);

				path = _engine_cache + thumbnail_filename;
				if (file_system_t::exists(path.c_str()))
					return read_file_range(path.c_str(), offset, size, out);
			}

			if (!_directory_path.empty())
			{
				string_t path = _directory_path + resource_filename;
				if (file_system_t::exists(path.c_str()))
					return read_file_range(path.c_str(), offset, size, out);

				path = _directory_path + thumbnail_filename;
				return read_file_range(path.c_str(), offset, size, out);
			}

			SFG_ERR("resource directory cache is not set: {0}", hash);
			return false;
		}

		if (_mode == mode_e::filepack)
		{
			auto it = _resource_map.find(hash);
			if (it == _resource_map.end())
			{
				SFG_ERR("resource not found in file pack: {0}", hash);
				return false;
			}

			const resource_map_info_t& info		 = it->second;
			const size_t			   read_size = size == 0 ? info.size : size;
			return read_file_range(_file_pack_path.c_str(), info.offset + offset, read_size, out);
		}

		SFG_ERR("resource file system mode is not set");
		return false;
	}

	bool resource_file_system_t::read_file_range(const char* path, size_t offset, size_t size, ostream_t& out)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("resource file not found: {0}", path);
			return false;
		}

		std::error_code ec;
		const size_t	file_size = static_cast<size_t>(std::filesystem::file_size(path, ec));
		if (ec)
		{
			SFG_ERR("failed to get resource file size: {0}", path);
			return false;
		}

		if (offset > file_size)
		{
			SFG_ERR("resource file offset out of range: {0}", path);
			return false;
		}

		const size_t read_size = size == 0 ? file_size - offset : size;
		if (read_size > file_size - offset)
		{
			SFG_ERR("resource file read size out of range: {0}", path);
			return false;
		}

		if (read_size == 0)
			return true;

		istream_t stream = serializer_t::load_from_file_slice(path, offset, read_size);
		if (stream.empty())
		{
			SFG_ERR("failed to read resource file slice: {0}", path);
			return false;
		}

		out.write_raw(stream.get_raw(), stream.get_size());
		return true;
	}
}
