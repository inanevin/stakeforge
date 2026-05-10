// Copyright (c) 2025 Inan Evin

#include "shader_cook_variants.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/blend_attachments.hpp>
#include <sfg/gfx/common/vertex_inputs.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	namespace
	{
		bool compile_stage(u8 stage_u8, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths, const char* entry, cook_stage_blob_t& out)
		{
			gfx_backend& backend = gfx_backend::get();
			span_t<u8>	 blob	 = {};
			span_t<u8>	 dummy	 = {};

			if (!backend.compile_shader_vertex_pixel(stage_u8, source, defines, include_paths, entry, blob, false, dummy))
				return false;

			out.stage = stage_u8;
			out.bytes.resize(blob.size);
			SFG_MEMCPY(out.bytes.data(), blob.data, blob.size);
			delete[] blob.data;
			return true;
		}

		bool add_compile_variant_vs_ps(cook_compile_variant_t& cv, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths)
		{
			cv.stages.push_back({});
			if (!compile_stage(static_cast<u8>(shader_stage_e::vertex), source, defines, include_paths, "VSMain", cv.stages.back()))
				return false;

			cv.stages.push_back({});
			if (!compile_stage(static_cast<u8>(shader_stage_e::fragment), source, defines, include_paths, "PSMain", cv.stages.back()))
				return false;

			return true;
		}

		bool cook_editor_ui_with_blend(const string_t& source, const vector_t<string_t>& include_paths, const color_blend_attachment_t& blend, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
		{
			out_compiles.push_back({});
			if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
				return false;

			shader_desc_t desc						  = {};
			desc.topo								  = topology::triangle_list;
			desc.cull								  = cull_mode::back;
			desc.front								  = front_face::cw;
			desc.fill								  = fill_mode::solid;
			desc.poly_mode							  = polygon_mode::fill;
			desc.samples							  = 1;
			desc.depth_stencil_desc.attachment_format = format_e::undefined;
			desc.depth_stencil_desc.flags			  = 0;

			vertex_inputs_t::get_editor_ui(desc);

			shader_color_attachment_t att = {
				.format			  = format_e::b8g8r8a8_srgb,
				.blend_attachment = blend,
			};
			desc.add_attachment(att);

			out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
			return true;
		}
	}

	bool shader_cook_variants_t::cook_editor_ui_default(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_editor_ui_lcd_text(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_lcd_text(), out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_editor_ui_text_grayscale(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_editor_ui_sdf(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), out_compiles, out_psos);
	}
}
