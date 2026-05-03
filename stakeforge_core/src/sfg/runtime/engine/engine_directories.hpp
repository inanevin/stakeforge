// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/string.hpp>

namespace sfg
{
	class engine_directories_t
	{
	public:
		static string_t get_user_directory();
		static string_t get_engine_assets();
		static string_t get_engine_resource_cache();
		static string_t get_engine_manifest();
	};
}
