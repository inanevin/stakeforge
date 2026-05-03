// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct cooking_options_t
	{
		string_t arguments;
	};

	enum class cook_result_e : u8
	{
		success,
		invalid_path,
		unsupported_extension,
		unsupported_schema,
		invalid_meta_file,
		cook_failed,
	};

	enum class cook_kind_e : u8
	{
		invalid,
		texture,
		audio,
		glb,
		font,
		shader,
		material,
		particle,
		sampler,
		physical_material,
		animation_state_machine,
		prefab,
		count,
	};

	cook_result_e cook_resource(const char* full_path, const cooking_options_t& options, ostream_t& stream);
	cook_result_e cook_resource(const char* full_path, const char* output_directory, const cooking_options_t& options);
}
