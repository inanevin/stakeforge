// Copyright (c) 2025 Inan Evin

#include "shader_cook_variants.hpp"
#include "shader_types.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/blend_attachments.hpp>
#include <sfg/gfx/common/vertex_inputs.hpp>
#include <sfg/io/log.hpp>
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
			{
				SFG_ERR("failed to compile shader stage {0} entry {1}", stage_u8, entry);
				return false;
			}

			out.stage = stage_u8;
			out.bytes.resize(blob.size);
			SFG_MEMCPY(out.bytes.data(), blob.data, blob.size);
			delete[] blob.data;
			return true;
		}

		bool add_compile_variant_vs_ps(cook_compile_variant_t& cv, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths, bool compile_ps = true)
		{
			cv.stages.push_back({});
			if (!compile_stage(static_cast<u8>(shader_stage_e::vertex), source, defines, include_paths, "VSMain", cv.stages.back()))
				return false;

			if (compile_ps)
			{
				cv.stages.push_back({});
				if (!compile_stage(static_cast<u8>(shader_stage_e::fragment), source, defines, include_paths, "PSMain", cv.stages.back()))
					return false;
			}

			return true;
		}

		void add_attachment(shader_desc_t& desc, format_e format, const color_blend_attachment_t& blend)
		{
			desc.add_attachment({
				.format			  = format,
				.blend_attachment = blend,
			});
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

		bool cook_shader_with_blend(
			const string_t& source, const vector_t<string_t>& include_paths, const color_blend_attachment_t& blend, u8 depth_flags, bool add_vertex_inputs, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
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
			desc.depth_stencil_desc.attachment_format = depth_flags == 0 ? format_e::undefined : format_e::d32_sfloat;
			desc.depth_stencil_desc.flags			  = depth_flags;

			if (add_vertex_inputs)
				vertex_inputs_t::get_pos_normal_tangent_uv(desc);

			shader_color_attachment_t att = {
				.format			  = format_e::b8g8r8a8_srgb,
				.blend_attachment = blend,
			};
			desc.add_attachment(att);

			out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
			return true;
		}

		bool add_compile_variant(vector_t<cook_compile_variant_t>& out_compiles, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& include_paths, bool compile_ps)
		{
			out_compiles.push_back({});
			if (!add_compile_variant_vs_ps(out_compiles.back(), source, defines, include_paths, compile_ps))
				return false;
			return true;
		}

		void add_gbuffer_pso(vector_t<cook_pso_variant_t>& out_psos, u8 compile_variant_index, u32 variant_flags)
		{
			const bitmask_t<u32> flags = variant_flags;
			shader_desc_t		 desc  = {};
			desc.topo				   = topology::triangle_list;
			desc.cull				   = flags.is_set(shader_variant_flags_double_sided) ? cull_mode::none : cull_mode::back;
			desc.front				   = front_face::ccw;
			desc.fill				   = fill_mode::solid;
			desc.poly_mode			   = polygon_mode::fill;
			desc.samples			   = 1;

			if (flags.is_set(shader_variant_flags_selection_outline))
			{
				add_attachment(desc, format_e::r8g8b8a8_unorm, blend_attachments_t::get_none());
			}
			else if (flags.is_set(shader_variant_flags_id_write))
			{
				add_attachment(desc, format_e::r32_uint, blend_attachments_t::get_none());
			}
			else
			{
				add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r10g0b10a2_unorm, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r8g8b8a8_unorm, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r16g16b16a16_sfloat, blend_attachments_t::get_none());
			}

			if (flags.is_set(shader_variant_flags_skinned))
				vertex_inputs_t::get_pos_normal_tangent_uv_skinned(desc);
			else
				vertex_inputs_t::get_pos_normal_tangent_uv(desc);

			bitmask_t<u8> depth_flags = flags.is_set(shader_variant_flags_selection_outline) ? 0 : dsf_depth_test;
			depth_flags.set(dsf_depth_write, flags.is_set(shader_variant_flags_z_prepass));
			desc.depth_stencil_desc = {
				.attachment_format = flags.is_set(shader_variant_flags_selection_outline) ? format_e::undefined : format_e::d32_sfloat,
				.depth_compare	   = flags.is_set(shader_variant_flags_shadow_rendering) ? compare_op::lequal : compare_op::gequal,
				.flags			   = depth_flags,
			};

			if (flags.is_set(shader_variant_flags_shadow_rendering))
			{
				desc.depth_bias_slope	 = 2.0f;
				desc.depth_bias_constant = 0.0f;
				desc.depth_bias_clamp	 = 0.0f;
			}

			out_psos.push_back({.desc = desc, .variant_flags = variant_flags, .compile_variant_index = compile_variant_index});
		}
	}

	bool shader_cook_variants_t::cook_opaque_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.reserve(16);
		out_psos.reserve(40);

		if (!add_compile_variant(out_compiles, source, {}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ZPREPASS"}, include_paths, false))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING", "USE_ZPREPASS"}, include_paths, false))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING", "USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_ALPHA_CUTOFF", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_ALPHA_CUTOFF", "USE_SKINNING"}, include_paths, true))
			return false;

		add_gbuffer_pso(out_psos, 0, 0);
		add_gbuffer_pso(out_psos, 1, shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 2, shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 3, shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 3, shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 4, shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 5, shader_variant_flags_skinned | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 5, shader_variant_flags_skinned | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 6, shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 6, shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 7, shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 7, shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 0, shader_variant_flags_double_sided);
		add_gbuffer_pso(out_psos, 1, shader_variant_flags_double_sided | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 2, shader_variant_flags_double_sided | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 3, shader_variant_flags_double_sided | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 3, shader_variant_flags_double_sided | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 4, shader_variant_flags_double_sided | shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 5, shader_variant_flags_double_sided | shader_variant_flags_skinned | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 5, shader_variant_flags_double_sided | shader_variant_flags_skinned | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 6, shader_variant_flags_double_sided | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 6, shader_variant_flags_double_sided | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 7, shader_variant_flags_double_sided | shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass);
		add_gbuffer_pso(out_psos, 7, shader_variant_flags_double_sided | shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff | shader_variant_flags_z_prepass | shader_variant_flags_shadow_rendering);
		add_gbuffer_pso(out_psos, 8, shader_variant_flags_id_write);
		add_gbuffer_pso(out_psos, 9, shader_variant_flags_id_write | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 9, shader_variant_flags_id_write | shader_variant_flags_alpha_cutoff | shader_variant_flags_double_sided);
		add_gbuffer_pso(out_psos, 10, shader_variant_flags_id_write | shader_variant_flags_alpha_cutoff | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 10, shader_variant_flags_id_write | shader_variant_flags_alpha_cutoff | shader_variant_flags_double_sided | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 8, shader_variant_flags_id_write | shader_variant_flags_double_sided);
		add_gbuffer_pso(out_psos, 11, shader_variant_flags_id_write | shader_variant_flags_double_sided | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 11, shader_variant_flags_id_write | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 12, shader_variant_flags_selection_outline);
		add_gbuffer_pso(out_psos, 13, shader_variant_flags_selection_outline | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 14, shader_variant_flags_selection_outline | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 15, shader_variant_flags_selection_outline | shader_variant_flags_alpha_cutoff | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 12, shader_variant_flags_selection_outline | shader_variant_flags_double_sided);
		add_gbuffer_pso(out_psos, 13, shader_variant_flags_selection_outline | shader_variant_flags_double_sided | shader_variant_flags_skinned);
		add_gbuffer_pso(out_psos, 14, shader_variant_flags_selection_outline | shader_variant_flags_double_sided | shader_variant_flags_alpha_cutoff);
		add_gbuffer_pso(out_psos, 15, shader_variant_flags_selection_outline | shader_variant_flags_double_sided | shader_variant_flags_alpha_cutoff | shader_variant_flags_skinned);
		return true;
	}

	bool shader_cook_variants_t::cook_transparent_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		if (!cook_shader_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), dsf_depth_test, true, out_compiles, out_psos))
			return false;

		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {"USE_SELECTION"}, include_paths))
			return false;
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {"USE_SELECTION", "USE_ALPHA_CUTOFF"}, include_paths))
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
		vertex_inputs_t::get_pos_normal_tangent_uv(desc);
		add_attachment(desc, format_e::r8g8b8a8_unorm, blend_attachments_t::get_none());
		out_psos.push_back({.desc = desc, .variant_flags = shader_variant_flags_selection_outline, .compile_variant_index = static_cast<u8>(out_compiles.size() - 2)});
		out_psos.push_back({.desc = desc, .variant_flags = shader_variant_flags_selection_outline | shader_variant_flags_alpha_cutoff, .compile_variant_index = static_cast<u8>(out_compiles.size() - 1)});
		return true;
	}

	bool shader_cook_variants_t::cook_post_process_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_shader_with_blend(source, include_paths, blend_attachments_t::get_none(), 0, false, out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_ui_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_ui_text_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		return cook_editor_ui_with_blend(source, include_paths, blend_attachments_t::get_alpha_blend(), out_compiles, out_psos);
	}

	bool shader_cook_variants_t::cook_deferred_lighting_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		add_attachment(desc, format_e::r16g16b16a16_sfloat, blend_attachments_t::get_none());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_post_combiner_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_none());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_texture_blit_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_none());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_editor_gizmo_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::back;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		vertex_inputs_t::get_pos_normal_tangent_uv(desc);
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_alpha_blend());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_debug_line_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		vertex_inputs_t::get_line_3d(desc);
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_alpha_blend());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_debug_text_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::solid;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		vertex_inputs_t::get_debug_text(desc);
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_alpha_blend());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
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
