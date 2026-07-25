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

		bool add_compile_variant_compute(cook_compile_variant_t& cv, const string_t& source, const vector_t<string_t>& include_paths)
		{
			gfx_backend& backend = gfx_backend::get();
			span_t<u8>	 blob	 = {};
			span_t<u8>	 dummy	 = {};
			if (!backend.compile_shader_compute(source, include_paths, "CSMain", blob, false, dummy))
			{
				SFG_ERR("failed to compile compute shader entry CSMain");
				return false;
			}

			cook_stage_blob_t& stage = cv.stages.emplace_back();
			stage.stage				 = static_cast<u8>(shader_stage_e::compute);
			stage.bytes.resize(blob.size);
			SFG_MEMCPY(stage.bytes.data(), blob.data, blob.size);
			delete[] blob.data;
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

		void add_object_pso(vector_t<cook_pso_variant_t>& out_psos, u8 compile_variant_index, u32 variant_flags)
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
			else if (flags.is_set(shader_variant_flags_gbuffer))
			{
				add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r10g0b10a2_unorm, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r8g8b8a8_unorm, blend_attachments_t::get_none());
				add_attachment(desc, format_e::r16g16b16a16_sfloat, blend_attachments_t::get_none());
			}
			else if (!flags.is_set(shader_variant_flags_z_prepass))
			{
				add_attachment(desc, format_e::r16g16b16a16_sfloat, blend_attachments_t::get_alpha_blend());
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

	bool shader_cook_variants_t::cook_object_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.reserve(20);
		out_psos.reserve(48);

		if (!add_compile_variant(out_compiles, source, {}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_GBUFFER"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_GBUFFER", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_GBUFFER", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_GBUFFER", "USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ZPREPASS"}, include_paths, false))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ZPREPASS", "USE_SKINNING"}, include_paths, false))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ZPREPASS", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_ZPREPASS", "USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"WRITE_ID", "USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_SKINNING"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;
		if (!add_compile_variant(out_compiles, source, {"USE_SELECTION", "USE_SKINNING", "USE_ALPHA_CUTOFF"}, include_paths, true))
			return false;

		const u32 surface_flags[4] = {
			0,
			shader_variant_flags_skinned,
			shader_variant_flags_alpha_cutoff,
			shader_variant_flags_skinned | shader_variant_flags_alpha_cutoff,
		};

		for (u8 i = 0; i < std::size(surface_flags); ++i)
		{
			const u32 flags = surface_flags[i];

			add_object_pso(out_psos, i, flags);
			add_object_pso(out_psos, i, flags | shader_variant_flags_double_sided);
			add_object_pso(out_psos, static_cast<u8>(4 + i), flags | shader_variant_flags_gbuffer);
			add_object_pso(out_psos, static_cast<u8>(4 + i), flags | shader_variant_flags_gbuffer | shader_variant_flags_double_sided);
		}

		for (u8 i = 0; i < std::size(surface_flags); ++i)
		{
			const u32 flags			  = surface_flags[i] | shader_variant_flags_z_prepass;
			const u8  compile_variant = static_cast<u8>(8 + i);

			add_object_pso(out_psos, compile_variant, flags);
			add_object_pso(out_psos, compile_variant, flags | shader_variant_flags_double_sided);
			add_object_pso(out_psos, compile_variant, flags | shader_variant_flags_shadow_rendering);
			add_object_pso(out_psos, compile_variant, flags | shader_variant_flags_shadow_rendering | shader_variant_flags_double_sided);
		}

		for (u8 i = 0; i < std::size(surface_flags); ++i)
		{
			const u32 flags			  = surface_flags[i] | shader_variant_flags_id_write;
			const u8  compile_variant = static_cast<u8>(12 + i);

			add_object_pso(out_psos, compile_variant, flags);
			add_object_pso(out_psos, compile_variant, flags | shader_variant_flags_double_sided);
		}

		for (u8 i = 0; i < std::size(surface_flags); ++i)
		{
			const u32 flags			  = surface_flags[i] | shader_variant_flags_selection_outline;
			const u8  compile_variant = static_cast<u8>(16 + i);

			add_object_pso(out_psos, compile_variant, flags);
			add_object_pso(out_psos, compile_variant, flags | shader_variant_flags_double_sided);
		}

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

	bool shader_cook_variants_t::cook_skybox_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});

		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.depth_stencil_desc.attachment_format = format_e::d32_sfloat;
		desc.depth_stencil_desc.depth_compare	  = compare_op::gequal;
		desc.depth_stencil_desc.flags			  = dsf_depth_test;
		desc.topo								  = topology::triangle_list;
		desc.fill								  = fill_mode::solid;
		desc.cull								  = cull_mode::none;
		desc.front								  = front_face::ccw;
		desc.poly_mode							  = polygon_mode::fill;
		desc.samples							  = 1;
		add_attachment(desc, format_e::r16g16b16a16_sfloat, blend_attachments_t::get_none());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
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

	bool shader_cook_variants_t::cook_debug_triangle_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_vs_ps(out_compiles.back(), source, {}, include_paths))
			return false;

		shader_desc_t desc						  = {};
		desc.topo								  = topology::triangle_list;
		desc.cull								  = cull_mode::back;
		desc.front								  = front_face::ccw;
		desc.fill								  = fill_mode::wireframe;
		desc.poly_mode							  = polygon_mode::fill;
		desc.depth_bias_constant				  = 0.01f;
		desc.depth_bias_slope					  = 1.0f;
		desc.samples							  = 1;
		desc.depth_stencil_desc.attachment_format = format_e::undefined;
		desc.depth_stencil_desc.flags			  = 0;
		vertex_inputs_t::get_pos_color(desc);
		add_attachment(desc, format_e::r8g8b8a8_srgb, blend_attachments_t::get_none());

		out_psos.push_back({.desc = desc, .variant_flags = 0, .compile_variant_index = 0});
		return true;
	}

	bool shader_cook_variants_t::cook_compute_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos)
	{
		out_compiles.push_back({});
		if (!add_compile_variant_compute(out_compiles.back(), source, include_paths))
			return false;

		out_psos.push_back({.desc = {}, .variant_flags = 0, .compile_variant_index = 0});
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
