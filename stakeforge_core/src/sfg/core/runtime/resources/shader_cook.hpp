// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/common/type_id.hpp"

#include "core/runtime/resources/shader_data_definition.hpp"
#include "core/runtime/resources/shader_types.hpp"
#include "core/data/string.hpp"
#include "core/data/vector.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct shader_cook_config_t
	{
		vector_t<string_t> include_dirs = {};
		shader_type_e	   type			= shader_type_e::invalid;
	};

	class SFG_API shader_cooker
	{
	public:
		static bool cook_from_file(const shader_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream, shader_data_definition_t& out_definition);
		static u64	collect_source_tick(const shader_cook_config_t& cfg, const char* full_path);
		static u64	collect_source_tick(const char* full_path);
	};

	SFG_DEFINE_TYPE_ID(shader_cook_config_t);

	struct shader_cook_config_reflection_t
	{
		shader_cook_config_reflection_t();
	};

}
