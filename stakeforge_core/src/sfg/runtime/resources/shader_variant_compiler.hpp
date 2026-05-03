// Copyright (c) 2025 Inan Evin
#pragma once

#include "shader_types.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct shader_compile_blob_t
	{
		vector_t<u8> bytes;
		u8			 stage = 0;
	};

	struct shader_compile_variant_scratch_t
	{
		vector_t<shader_compile_blob_t> blobs;
	};

	struct shader_pso_variant_scratch_t
	{
		u32 variant_flags		  = 0;
		u8	compile_variant_index = 0;
	};

	struct shader_compile_t
	{
		vector_t<shader_compile_variant_scratch_t> compile_variants;
		vector_t<shader_pso_variant_scratch_t>	   pso_variants;
		shader_type_e							   type = shader_type_e::invalid;
	};

	namespace shader_variant_compiler
	{
		bool compile(shader_type_e type, const string_t& source, const vector_t<string_t>& include_paths, shader_compile_t& out);
	}
}
