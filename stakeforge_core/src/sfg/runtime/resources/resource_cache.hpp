// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	class ostream_t;

	namespace resource_cache
	{
		bool ensure_directory(const char* dir);
		bool try_load_fresh(const char* cache_dir, const char* name, const char* source_path, vector_t<u8>& out_bytes);
		bool save(const char* cache_dir, const char* name, const char* source_path, const ostream_t& cooked);
	}
}
