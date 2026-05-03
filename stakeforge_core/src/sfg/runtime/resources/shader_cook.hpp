// Copyright (c) 2025 Inan Evin
#pragma once

#include "shader.hpp"
#include "shader_variant_compiler.hpp"

namespace sfg
{
	class ostream_t;

	bool shader_cook_serialize(const shader_compile_t& src, ostream_t& stream);
}
