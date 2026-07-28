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
#include <fstream>
#include <limits>

namespace sfg
{
	resource_stream_t::~resource_stream_t()
	{
		close();
	}

	void resource_stream_t::close()
	{
		if (_stream == nullptr)
			return;

		std::ifstream* stream = static_cast<std::ifstream*>(_stream);
		stream->close();
		delete stream;

		_stream		 = nullptr;
		_base_offset = 0;
		_size		 = 0;
		_cursor		 = 0;
	}

	bool resource_stream_t::read(void* destination, size_t size, size_t& out_read)
	{
		SFG_ASSERT(_stream != nullptr);
		SFG_ASSERT(destination != nullptr);

		const u64 remaining = _size - _cursor;
		const u64 read_size = size < remaining ? size : remaining;
		out_read			= static_cast<size_t>(read_size);

		if (read_size == 0)
			return true;

		std::ifstream& stream = *static_cast<std::ifstream*>(_stream);
		stream.clear();
		stream.seekg(static_cast<std::streamoff>(_base_offset + _cursor), std::ios::beg);
		stream.read(static_cast<char*>(destination), static_cast<std::streamsize>(read_size));

		if (stream.gcount() != static_cast<std::streamsize>(read_size))
		{
			out_read = static_cast<size_t>(stream.gcount());
			_cursor += out_read;
			return false;
		}

		_cursor += read_size;
		return true;
	}

	bool resource_stream_t::seek(i64 offset, resource_seek_origin_e origin)
	{
		SFG_ASSERT(_stream != nullptr);

		i64 cursor = 0;

		switch (origin)
		{
		case resource_seek_origin_e::start:
			cursor = offset;
			break;
		case resource_seek_origin_e::current:
			cursor = static_cast<i64>(_cursor) + offset;
			break;
		case resource_seek_origin_e::end:
			cursor = static_cast<i64>(_size) + offset;
			break;
		}

		if (cursor < 0 || static_cast<u64>(cursor) > _size)
			return false;

		_cursor = static_cast<u64>(cursor);
		return true;
	}

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
			const string_t filename_base	 = std::to_string(hash);
			const string_t resource_filename = filename_base + ".sfg_bin";
			if (!_engine_cache.empty())
			{
				string_t path = _engine_cache + resource_filename;
				if (file_system_t::exists(path.c_str()))
					return read_file_range(path.c_str(), offset, size, out);
			}

			if (!_directory_path.empty())
			{
				string_t path = _directory_path + resource_filename;
				if (file_system_t::exists(path.c_str()))
					return read_file_range(path.c_str(), offset, size, out);
				else
				{
					SFG_ERR("resource not found: {0}", path.c_str());
				}

				return false;
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

	bool resource_file_system_t::open_resource_stream(u64 hash, size_t offset, size_t size, resource_stream_t& out) const
	{
		SFG_ASSERT(!out.is_open());

		string_t path		  = {};
		u64		 range_offset = 0;
		u64		 range_size	  = 0;

		if (!resolve_resource_range(hash, offset, size, path, range_offset, range_size))
			return false;

		std::ifstream* stream = new std::ifstream(path.c_str(), std::ios::binary);

		if (!stream->is_open())
		{
			SFG_ERR("failed to open resource stream: {0}", path.c_str());
			delete stream;
			return false;
		}

		out._stream		 = stream;
		out._base_offset = range_offset;
		out._size		 = range_size;
		out._cursor		 = 0;

		return true;
	}

	bool resource_file_system_t::resolve_resource_range(u64 hash, size_t offset, size_t size, string_t& out_path, u64& out_offset, u64& out_size) const
	{
		u64 resource_offset = 0;
		u64 resource_size	= 0;

		if (_mode == mode_e::directory)
		{
			const string_t filename_base	 = std::to_string(hash);
			const string_t resource_filename = filename_base + ".sfg_bin";

			if (!_engine_cache.empty())
			{
				const string_t engine_path = _engine_cache + resource_filename;

				if (file_system_t::exists(engine_path.c_str()))
					out_path = engine_path;
			}

			if (out_path.empty() && !_directory_path.empty())
			{
				const string_t directory_path = _directory_path + resource_filename;

				if (file_system_t::exists(directory_path.c_str()))
					out_path = directory_path;
			}

			if (out_path.empty())
			{
				SFG_ERR("resource not found for streaming: {0}", hash);
				return false;
			}

			resource_size = file_system_t::get_file_size(out_path.c_str());
		}
		else if (_mode == mode_e::filepack)
		{
			const auto it = _resource_map.find(hash);

			if (it == _resource_map.end())
			{
				SFG_ERR("resource not found in file pack for streaming: {0}", hash);
				return false;
			}

			out_path		= _file_pack_path;
			resource_offset = it->second.offset;
			resource_size	= it->second.size;
		}
		else
		{
			SFG_ERR("resource file system mode is not set");
			return false;
		}

		if (offset > resource_size)
		{
			SFG_ERR("resource stream offset out of range: {0}", hash);
			return false;
		}

		const u64 range_size = size == 0 ? resource_size - offset : size;

		if (range_size > resource_size - offset)
		{
			SFG_ERR("resource stream size out of range: {0}", hash);
			return false;
		}

		out_offset = resource_offset + offset;
		out_size   = range_size;
		return true;
	}

	bool resource_file_system_t::read_file_range(const char* path, size_t offset, size_t size, ostream_t& out)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("resource file not found: {0}", path);
			return false;
		}

		std::error_code ec		  = {};
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
