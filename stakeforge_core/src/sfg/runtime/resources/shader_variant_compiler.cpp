// Copyright (c) 2025 Inan Evin

#include "shader_variant_compiler.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	namespace
	{
		bool compile_stage(u8 stage_u8, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths, const char* entry, shader_compile_blob_t& out)
		{
			gfx_backend* backend = gfx_backend::get();
			span_t<u8>	 blob	 = {};
			span_t<u8>	 dummy	 = {};

			if (!backend->compile_shader_vertex_pixel(stage_u8, source, defines, include_paths, entry, blob, false, dummy))
				return false;

			out.stage = stage_u8;
			out.bytes.resize(blob.size);
			SFG_MEMCPY(out.bytes.data(), blob.data, blob.size);
			delete[] blob.data;
			return true;
		}

		bool add_compile_variant_vs_ps(shader_compile_t& out, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths)
		{
			out.compile_variants.push_back({});
			shader_compile_variant_scratch_t& cv = out.compile_variants.back();

			cv.blobs.push_back({});
			if (!compile_stage(static_cast<u8>(shader_stage::vertex), source, defines, include_paths, "VSMain", cv.blobs.back()))
				return false;

			cv.blobs.push_back({});
			if (!compile_stage(static_cast<u8>(shader_stage::fragment), source, defines, include_paths, "PSMain", cv.blobs.back()))
				return false;

			return true;
		}

		void add_pso(shader_compile_t& out, u8 compile_index, u32 flags)
		{
			out.pso_variants.push_back({.variant_flags = flags, .compile_variant_index = compile_index});
		}

		bool compile_style_simple(const string_t& source, const vector_t<string_t>& include_paths, shader_compile_t& out)
		{
			out.compile_variants.clear();
			out.pso_variants.clear();

			if (!add_compile_variant_vs_ps(out, source, {}, include_paths))
				return false;

			add_pso(out, 0, 0);
			return true;
		}
	}

	bool shader_variant_compiler::compile(shader_type_e type, const string_t& source, const vector_t<string_t>& include_paths, shader_compile_t& out)
	{
		out.type = type;
		switch (type)
		{
		case shader_type_e::editor_ui_default:
		case shader_type_e::editor_ui_text:
		case shader_type_e::editor_ui_sdf:
			return compile_style_simple(source, include_paths, out);
		default:
			SFG_ERR("shader_variant_compiler: unsupported shader type {0}", static_cast<u8>(type));
			return false;
		}
	}
}
