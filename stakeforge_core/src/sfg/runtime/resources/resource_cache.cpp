// Copyright (c) 2025 Inan Evin

#include "resource_cache.hpp"
#include "common_resources.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	namespace
	{
		string_t cache_path(const char* dir, const char* name)
		{
			string_t p = dir;
			file_system_t::fix_path(p);
			if (!p.empty() && p.back() != '/')
				p += '/';
			p += name;
			p += ".sfg_cache";
			return p;
		}
	}

	bool resource_cache_t::ensure_directory(const char* cache_dir)
	{
		if (file_system_t::is_directory(cache_dir))
			return true;
		return file_system_t::create_directory(cache_dir);
	}

	istream_t resource_cache_t::try_load(const char* cache_dir, const char* name, const resource_header_t& expected)
	{
		const string_t path = cache_path(cache_dir, name);

		if (!file_system_t::exists(path.c_str()))
			return {};

		istream_t stream = serializer_t::load_from_file(path.c_str());
		if (stream.empty())
			return {};

		resource_header_t header = {};
		header.deserialize(stream);
		if (header.magic != expected.magic || header.version != expected.version || header.modified_ticks != expected.modified_ticks)
			return {};

		stream.seek(0);
		return stream;
	}

	bool resource_cache_t::save(const char* cache_dir, const char* name, const ostream_t& cooked)
	{
		if (!resource_cache_t::ensure_directory(cache_dir))
		{
			SFG_ERR("failed to create directory {0}", cache_dir);
			return false;
		}

		const string_t path = cache_path(cache_dir, name);
		if (!serializer_t::save_to_file(path.c_str(), cooked))
		{
			SFG_ERR("failed to write cache {0}", path.c_str());
			return false;
		}
		return true;
	}
}
