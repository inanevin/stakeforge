// Copyright (c) 2025 Inan Evin
#pragma once

#include "shader.hpp"
#include "shader_types.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	class ostream_t;

	struct shader_cook_config_t
	{
		vector_t<string_t> include_dirs = {};
		shader_type_e	   type			= shader_type_e::invalid;
	};

	class shader_cooker
	{
	public:
		static bool cook_from_file(const shader_cook_config_t& cfg, const char* full_path, ostream_t& stream);
		static void collect_source_ticks(const shader_cook_config_t& cfg, const char* full_path, vector_t<u64>& out);
		static void collect_source_ticks(const char* full_path, vector_t<u64>& out);
	};

}
