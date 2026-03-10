/*
/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "shader_variant_compiler.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/shader_description.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/common/vertex_inputs.hpp"
#include "gfx/common/blend_definitions.hpp"
#include "resources/common_resources.hpp"
#include "shader_raw.hpp"
#
namespace SFG
{
	bool shader_variant_compiler::compile(const compile_params& params)
	{
		gfx_backend* backend = gfx_backend::get();
		return backend->compile_shader_vertex_pixel(static_cast<shader_stage>(params.stage), params.text, params.defines, params.folder_paths, params.entry.c_str(), params.data, params.compile_layout, params.out_layout);
	}

	bool shader_variant_compiler::compile_compute(const compile_params& p)
	{
		gfx_backend* backend = gfx_backend::get();
		return backend->compile_shader_compute(p.text, p.folder_paths, p.entry.c_str(), p.data, p.compile_layout, p.out_layout);
	}

	bool shader_variant_compiler::compile_raw(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths, const shader_desc& desc)
	{
		/*
			Compile one compile variant depending on shader entries.
			Add one PSO completely based on raw description from the shader file.
		*/

		raw.compile_variants.push_back({});
		compile_variant& def_compile = raw.compile_variants.back();
		span<u8>		 dummy_layout;
		if (!desc.vertex_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			const bool res = compile({
				.stage			= static_cast<u8>(shader_stage::vertex),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.vertex_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		if (!desc.pixel_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			const bool res = compile({
				.stage			= static_cast<u8>(shader_stage::fragment),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.pixel_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		if (!desc.compute_entry.empty())
		{
			def_compile.blobs.push_back({.stage = shader_stage::compute});

			const bool res = compile_compute({
				.stage			= static_cast<u8>(shader_stage::compute),
				.data			= def_compile.blobs.back().data,
				.defines		= {},
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= desc.compute_entry,
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}
		}

		raw.pso_variants.push_back({});
		pso_variant& pso	= raw.pso_variants.back();
		pso.desc			= desc;
		pso.compile_variant = 0;
		pso.desc.debug_name = raw.name;
		return true;
	}

	bool shader_variant_compiler::compile_style_gbuffer_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs		   = {};
		vector<vertex_input> vertex_inputs_skinned = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv, vertex_inputs);
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv_skinned, vertex_inputs_skinned);

		const vector<shader_color_attachment> color_attachments = {
			{
				.format			  = render_target_definitions::get_format_gbuffer_albedo(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_normal(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_orm(),
				.blend_attachment = blend_attachment,
			},
			{
				.format			  = render_target_definitions::get_format_gbuffer_emissive(),
				.blend_attachment = blend_attachment,
			},
		};

		auto add_compile_var = [&](const vector<string>& defines, bool compile_ps) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			if (compile_ps)
			{

				def_compile.blobs.push_back({.stage = shader_stage::fragment});
				res = compile({
					.stage			= static_cast<u8>(def_compile.blobs.back().stage),
					.data			= def_compile.blobs.back().data,
					.defines		= defines,
					.text			= shader_text,
					.folder_paths	= folder_paths,
					.compile_layout = false,
					.out_layout		= dummy_layout,
					.entry			= "PSMain",
				});

				if (!res)
				{
					def_compile.destroy();
					return false;
				}
			}

			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = variant_flags.is_set(variant_flag_skinned) ? vertex_inputs_skinned : vertex_inputs;
			pso.desc.cull		 = variant_flags.is_set(variant_flag_double_sided) ? cull_mode::none : cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			if (variant_flags.is_set(variant_flag_id_write))
			{
				pso.desc.attachments = {{
					.format = render_target_definitions::get_format_object_id(),
				}};
			}

			bitmask<u8> depth_flags = depth_stencil_flags::dsf_depth_test;
			depth_flags.set(depth_stencil_flags::dsf_depth_write, variant_flags.is_set(shader_variant_flags::variant_flag_z_prepass));

			const compare_op cmp = variant_flags.is_set(shader_variant_flags::variant_flag_shadow_rendering) ? compare_op::lequal : compare_op::gequal;

			if (variant_flags.is_set(shader_variant_flags::variant_flag_shadow_rendering))
			{
				pso.desc.depth_bias_slope	 = 2.0;
				pso.desc.depth_bias_constant = 0;
				pso.desc.depth_bias_clamp	 = 0;
			}

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = cmp,
				.flags			   = depth_flags,
			};
		};

		// Compile-time variants.
		if (!add_compile_var({}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING"}, true))
			return false;
		if (!add_compile_var({"USE_ALPHA_CUTOFF"}, true))
			return false;
		if (!add_compile_var({"USE_ZPREPASS"}, false))
			return false;

		if (!add_compile_var({"USE_SKINNING", "USE_ALPHA_CUTOFF"}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING", "USE_ZPREPASS"}, false))
			return false;
		if (!add_compile_var({"USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING", "USE_ALPHA_CUTOFF", "USE_ZPREPASS"}, true))
			return false;

		if (!add_compile_var({"WRITE_ID"}, true))
			return false;

		if (!add_compile_var({"WRITE_ID", "USE_ALPHA_CUTOFF"}, true))
			return false;

		if (!add_compile_var({"WRITE_ID", "USE_ALPHA_CUTOFF", "USE_SKINNING"}, true))
			return false;

		if (!add_compile_var({"WRITE_ID", "USE_SKINNING"}, true))
			return false;

		// PSO-variants
		add_pso(0, 0);
		add_pso(1, variant_flag_skinned);
		add_pso(2, variant_flag_alpha_cutoff);
		add_pso(3, variant_flag_z_prepass);
		add_pso(3, variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(4, variant_flag_skinned | variant_flag_alpha_cutoff);
		add_pso(5, variant_flag_skinned | variant_flag_z_prepass);
		add_pso(5, variant_flag_skinned | variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(6, variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(6, variant_flag_alpha_cutoff | variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(7, variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(7, variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass | variant_flag_shadow_rendering);

		// double-sided
		add_pso(0, variant_flag_double_sided);
		add_pso(1, variant_flag_double_sided | variant_flag_skinned);
		add_pso(2, variant_flag_double_sided | variant_flag_alpha_cutoff);
		add_pso(3, variant_flag_double_sided | variant_flag_z_prepass);
		add_pso(3, variant_flag_double_sided | variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(4, variant_flag_double_sided | variant_flag_skinned | variant_flag_alpha_cutoff);
		add_pso(5, variant_flag_double_sided | variant_flag_skinned | variant_flag_z_prepass);
		add_pso(5, variant_flag_double_sided | variant_flag_skinned | variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(6, variant_flag_double_sided | variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(6, variant_flag_double_sided | variant_flag_alpha_cutoff | variant_flag_z_prepass | variant_flag_shadow_rendering);
		add_pso(7, variant_flag_double_sided | variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass);
		add_pso(7, variant_flag_double_sided | variant_flag_skinned | variant_flag_alpha_cutoff | variant_flag_z_prepass | variant_flag_shadow_rendering);

		add_pso(8, variant_flag_id_write);
		add_pso(9, variant_flag_id_write | variant_flag_alpha_cutoff);
		add_pso(9, variant_flag_id_write | variant_flag_alpha_cutoff | variant_flag_double_sided);
		add_pso(10, variant_flag_id_write | variant_flag_alpha_cutoff | variant_flag_skinned);
		add_pso(10, variant_flag_id_write | variant_flag_alpha_cutoff | variant_flag_double_sided | variant_flag_skinned);

		add_pso(8, variant_flag_id_write | variant_flag_double_sided);
		add_pso(11, variant_flag_id_write | variant_flag_double_sided | variant_flag_skinned);
		add_pso(11, variant_flag_id_write | variant_flag_skinned);
		return true;
	}

	bool shader_variant_compiler::compile_style_forward_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::alpha_blend, blend_attachment);

		vector<vertex_input> vertex_inputs		   = {};
		vector<vertex_input> vertex_inputs_skinned = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv, vertex_inputs);
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv_skinned, vertex_inputs_skinned);

		const vector<shader_color_attachment> color_attachments = {
			{
				.format			  = render_target_definitions::get_format_lighting(),
				.blend_attachment = blend_attachment,
			},
		};

		auto add_compile_var = [&](const vector<string>& defines, bool compile_ps) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});

			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			if (compile_ps)
			{

				def_compile.blobs.push_back({.stage = shader_stage::fragment});
				res = compile({
					.stage			= static_cast<u8>(def_compile.blobs.back().stage),
					.data			= def_compile.blobs.back().data,
					.defines		= defines,
					.text			= shader_text,
					.folder_paths	= folder_paths,
					.compile_layout = false,
					.out_layout		= dummy_layout,
					.entry			= "PSMain",
				});

				if (!res)
				{
					def_compile.destroy();
					return false;
				}
			}

			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = variant_flags.is_set(variant_flag_skinned) ? vertex_inputs_skinned : vertex_inputs;
			pso.desc.cull		 = variant_flags.is_set(variant_flag_double_sided) ? cull_mode::none : cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			if (variant_flags.is_set(variant_flag_id_write))
			{
				pso.desc.attachments = {{
					.format = render_target_definitions::get_format_object_id(),
				}};
			}

			bitmask<u8> depth_flags = depth_stencil_flags::dsf_depth_test;

			const compare_op cmp = compare_op::gequal;

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = cmp,
				.flags			   = depth_flags,
			};
		};

		// Compile-time variants.
		if (!add_compile_var({}, true))
			return false;
		if (!add_compile_var({"USE_SKINNING"}, true))
			return false;
		if (!add_compile_var({"WRITE_ID"}, true))
			return false;

		if (!add_compile_var({"WRITE_ID", "USE_SKINNING"}, true))
			return false;

		// PSO-variants
		add_pso(0, 0);
		add_pso(1, variant_flag_skinned);

		// double-sided
		add_pso(0, variant_flag_double_sided);
		add_pso(1, variant_flag_double_sided | variant_flag_skinned);

		add_pso(2, variant_flag_id_write);
		add_pso(2, variant_flag_id_write | variant_flag_double_sided);
		add_pso(3, variant_flag_id_write | variant_flag_double_sided | variant_flag_skinned);
		add_pso(3, variant_flag_id_write | variant_flag_skinned);

		return true;
	}

	bool shader_variant_compiler::compile_style_object_outline(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs		   = {};
		vector<vertex_input> vertex_inputs_skinned = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv, vertex_inputs);
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv_skinned, vertex_inputs_skinned);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_selection(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = variant_flags.is_set(variant_flag_skinned) ? vertex_inputs_skinned : vertex_inputs;
			pso.desc.cull		 = variant_flags.is_set(variant_flag_double_sided) ? cull_mode::none : cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			pso.desc.depth_stencil_desc = {
				.flags = 0,
			};
		};

		if (!add_compile_var({}))
			return false;
		if (!add_compile_var({"USE_SKINNING"}))
			return false;

		add_pso(0, 0);
		add_pso(0, variant_flag_double_sided);
		add_pso(1, variant_flag_skinned);
		add_pso(1, variant_flag_skinned | variant_flag_double_sided);

		return true;
	}

	bool shader_variant_compiler::compile_style_post_combiner(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{

		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_post_combine(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = {};
			pso.desc.cull		 = cull_mode::none;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			pso.desc.depth_stencil_desc = {
				.flags = 0,
			};
		};

		if (!add_compile_var({}))
			return false;
		if (!add_compile_var({"USE_SELECTION_OUTLINE"}))
			return false;

		add_pso(0, 0);
		add_pso(1, variant_flag_selection_outline);

		return true;
	}

	bool shader_variant_compiler::compile_style_gizmo(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_normal_tangents_uv, vertex_inputs);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_post_combine(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = vertex_inputs;
			pso.desc.cull		 = cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			if (variant_flags.is_set(variant_flag_id_write))
			{
				pso.desc.attachments = {{
					.format = render_target_definitions::get_format_object_id(),
				}};
			}

			pso.desc.depth_stencil_desc = {
				.flags = 0,
			};
		};

		if (!add_compile_var({}))
			return false;

		if (!add_compile_var({"WRITE_ID"}))
			return false;

		add_pso(0, 0);
		add_pso(1, variant_flag_id_write);

		return true;
	}
	bool shader_variant_compiler::compile_style_debug_triangle(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::position_color, vertex_inputs);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_lighting(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso			 = raw.pso_variants.back();
			pso.compile_variant			 = compile_variant_index;
			pso.variant_flags			 = variant_flags;
			pso.desc.debug_name			 = raw.name;
			pso.desc.attachments		 = color_attachments;
			pso.desc.inputs				 = vertex_inputs;
			pso.desc.cull				 = cull_mode::back;
			pso.desc.topo				 = topology::triangle_list;
			pso.desc.front				 = front_face::ccw;
			pso.desc.poly_mode			 = polygon_mode::fill;
			pso.desc.depth_bias_constant = 0.01f;
			pso.desc.depth_bias_slope	 = 1;
			pso.desc.fill				 = fill_mode::wireframe;

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = compare_op::gequal,
				.flags			   = depth_stencil_flags::dsf_depth_test,

			};
		};

		if (!add_compile_var({}))
			return false;

		add_pso(0, 0);

		return true;
	}
	bool shader_variant_compiler::compile_style_debug_line(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		vector<vertex_input> vertex_inputs = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::line_3d, vertex_inputs);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_lighting(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso			 = raw.pso_variants.back();
			pso.compile_variant			 = compile_variant_index;
			pso.variant_flags			 = variant_flags;
			pso.desc.debug_name			 = raw.name;
			pso.desc.attachments		 = color_attachments;
			pso.desc.inputs				 = vertex_inputs;
			pso.desc.cull				 = cull_mode::none;
			pso.desc.topo				 = topology::triangle_list;
			pso.desc.front				 = front_face::ccw;
			pso.desc.poly_mode			 = polygon_mode::fill;
			pso.desc.depth_bias_constant = 0.5f;

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = compare_op::gequal,
				.flags			   = depth_stencil_flags::dsf_depth_test,
			};
		};

		if (!add_compile_var({}))
			return false;

		add_pso(0, 0);

		return true;
	}

	bool shader_variant_compiler::compile_style_gui(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{

		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::alpha_blend, blend_attachment);

		vector<vertex_input> vertex_inputs = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::gui, vertex_inputs);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_post_combine(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = vertex_inputs;
			pso.desc.cull		 = cull_mode::none;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = front_face::cw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			pso.desc.depth_stencil_desc = {

			};
		};

		if (!add_compile_var({}))
			return false;

		add_pso(0, 0);

		return true;
	}

	bool shader_variant_compiler::compile_style_engine_gui(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::src_alpha,
			.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_red | ccf_green | ccf_blue | ccf_alpha,
		};

		vector<vertex_input> vertex_inputs = {};
		vertex_inputs::get_vertex_inputs(vertex_input_style::gui, vertex_inputs);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_editor(), .blend_attachment = blend_attachment},
		}};

		const vector<shader_color_attachment> color_attachments_3d = {{
			{.format = render_target_definitions::get_format_lighting(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	= raw.pso_variants.back();
			pso.compile_variant = compile_variant_index;
			pso.variant_flags	= variant_flags;
			pso.desc.debug_name = raw.name;

			pso.desc.attachments = variant_flags.is_set(variant_flag_gui_3d) ? color_attachments_3d : color_attachments;
			pso.desc.inputs		 = vertex_inputs;
			pso.desc.cull		 = cull_mode::back;
			pso.desc.topo		 = topology::triangle_list;
			pso.desc.front		 = variant_flags.is_set(variant_flag_gui_3d) ? front_face::ccw : front_face::cw;
			pso.desc.poly_mode	 = polygon_mode::fill;

			pso.desc.depth_stencil_desc = {

			};
		};

		if (!add_compile_var({}))
			return false;

		if (!add_compile_var({"IS_3D"}))
			return false;

		add_pso(0, 0);
		add_pso(1, variant_flag_gui_3d);

		return true;
	}

	bool shader_variant_compiler::compile_style_particle_additive(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::additive, blend_attachment);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_lighting(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = {};
			pso.desc.cull		 = cull_mode::back;
			pso.desc.topo		 = topology::triangle_strip;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;
			pso.desc.fill		 = fill_mode::solid;

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = compare_op::gequal,
				.flags			   = depth_stencil_flags::dsf_depth_test,
			};
		};

		if (!add_compile_var({}))
			return false;

		add_pso(0, 0);

		return true;
	}

	bool shader_variant_compiler::compile_style_sprite(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::alpha_blend, blend_attachment);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_lighting(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = {};
			pso.desc.cull		 = cull_mode::back;
			pso.desc.topo		 = topology::triangle_strip;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;
			pso.desc.fill		 = fill_mode::solid;

			pso.desc.depth_stencil_desc = {
				.attachment_format = render_target_definitions::get_format_depth_default(),
				.depth_compare	   = compare_op::gequal,
				.flags			   = depth_stencil_flags::dsf_depth_test,
			};
		};

		if (!add_compile_var({}))
			return false;

		add_pso(0, 0);

		return true;
	}

	bool shader_variant_compiler::compile_style_swapchain(shader_raw& raw, const string& shader_text, const vector<string>& folder_paths)
	{
		color_blend_attachment blend_attachment = {};
		blend_definitions::get_blend_attachment(blend_definition_style::none, blend_attachment);

		const vector<shader_color_attachment> color_attachments = {{
			{.format = render_target_definitions::get_format_swapchain(), .blend_attachment = blend_attachment},
		}};

		auto add_compile_var = [&](const vector<string>& defines) -> bool {
			raw.compile_variants.push_back({});
			compile_variant& def_compile = raw.compile_variants.back();
			span<u8>		 dummy_layout;

			def_compile.blobs.push_back({.stage = shader_stage::vertex});
			bool res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "VSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}

			def_compile.blobs.push_back({.stage = shader_stage::fragment});
			res = compile({
				.stage			= static_cast<u8>(def_compile.blobs.back().stage),
				.data			= def_compile.blobs.back().data,
				.defines		= defines,
				.text			= shader_text,
				.folder_paths	= folder_paths,
				.compile_layout = false,
				.out_layout		= dummy_layout,
				.entry			= "PSMain",
			});
			if (!res)
			{
				def_compile.destroy();
				return false;
			}
			return true;
		};

		auto add_pso = [&](u32 compile_variant_index, const bitmask<u32>& variant_flags) {
			raw.pso_variants.push_back({});
			pso_variant& pso	 = raw.pso_variants.back();
			pso.compile_variant	 = compile_variant_index;
			pso.variant_flags	 = variant_flags;
			pso.desc.debug_name	 = raw.name;
			pso.desc.attachments = color_attachments;
			pso.desc.inputs		 = {};
			pso.desc.cull		 = cull_mode::back;
			pso.desc.topo		 = topology::triangle_strip;
			pso.desc.front		 = front_face::ccw;
			pso.desc.poly_mode	 = polygon_mode::fill;
			pso.desc.fill		 = fill_mode::solid;

			pso.desc.depth_stencil_desc = {

			};
		};

		if (!add_compile_var({}))
			return false;

		if (!add_compile_var({"SFG_CONSOLE"}))
			return false;

		if (!add_compile_var({"SFG_CONSOLE_AND_EDITOR"}))
			return false;

		add_pso(0, 0);
		add_pso(1, variant_flag_console);
		add_pso(2, variant_flag_console_and_editor);

		return true;
	}

}
