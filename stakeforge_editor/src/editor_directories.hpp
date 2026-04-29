// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/string.hpp"

namespace sfg
{
	class editor_directories_t
	{
	public:
		static string_t get_user_directory();
		static string_t get_settings_path();
		static string_t get_editor_assets();
	};
}
