// Copyright (c) 2025 Inan Evin

#include "resource_cache.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>

#include <cstdio>
#include <fstream>

namespace sfg
{
	namespace
	{
		string_t join(const char* dir, const char* name, const char* suffix)
		{
			string_t p = dir;
			file_system::fix_path(p);
			if (!p.empty() && p.back() != '/')
				p += '/';
			p += name;
			p += suffix;
			return p;
		}
	}

	bool resource_cache::ensure_directory(const char* dir)
	{
		if (file_system::is_directory(dir))
			return true;
		return file_system::create_directory(dir);
	}

	bool resource_cache::try_load_fresh(const char* cache_dir, const char* name, const char* source_path, vector_t<u8>& out_bytes)
	{
		const string_t data_path = join(cache_dir, name, ".data");
		const string_t meta_path = join(cache_dir, name, ".meta");

		if (!file_system::exists(data_path.c_str()) || !file_system::exists(meta_path.c_str()))
			return false;

		const string_t meta_str = file_system::read_file_as_string(meta_path.c_str());
		if (meta_str.empty())
			return false;

		u64		  cached_mtime = 0;
		const int parsed	   = std::sscanf(meta_str.c_str(), "%llu", &cached_mtime);
		if (parsed != 1)
			return false;

		const u64 source_mtime = file_system::get_last_modified_ticks(source_path);
		if (source_mtime == 0 || cached_mtime != source_mtime)
			return false;

		char*  raw_data = nullptr;
		size_t raw_size = 0;
		file_system::read_file(data_path.c_str(), raw_data, raw_size);
		if (raw_data == nullptr || raw_size == 0)
			return false;

		out_bytes.resize(raw_size);
		SFG_MEMCPY(out_bytes.data(), raw_data, raw_size);
		delete[] raw_data;
		return true;
	}

	bool resource_cache::save(const char* cache_dir, const char* name, const char* source_path, const ostream_t& cooked)
	{
		if (!ensure_directory(cache_dir))
		{
			SFG_ERR("resource_cache: failed to create directory {0}", cache_dir);
			return false;
		}

		const string_t data_path = join(cache_dir, name, ".data");
		const string_t meta_path = join(cache_dir, name, ".meta");

		std::ofstream df(data_path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (!df)
		{
			SFG_ERR("resource_cache: cannot open {0}", data_path.c_str());
			return false;
		}
		df.write(reinterpret_cast<const char*>(cooked.get_raw()), static_cast<std::streamsize>(cooked.get_size()));
		df.close();

		const u64 mtime = file_system::get_last_modified_ticks(source_path);

		std::ofstream mf(meta_path.c_str(), std::ios::out | std::ios::trunc);
		if (!mf)
		{
			SFG_ERR("resource_cache: cannot open {0}", meta_path.c_str());
			return false;
		}
		mf << mtime;
		mf.close();
		return true;
	}
}
