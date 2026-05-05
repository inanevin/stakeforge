// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;
	struct resource_header_t;

	class resource_cache_t
	{
	public:
		static bool		 ensure_directory(const char* cache_dir);
		static istream_t try_load(const char* cache_dir, const char* name, const resource_header_t& expected);
		static bool		 save(const char* cache_dir, const char* name, const ostream_t& cooked);
	};
}
