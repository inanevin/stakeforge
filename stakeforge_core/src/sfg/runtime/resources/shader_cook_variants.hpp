// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/shader_description.hpp>

namespace sfg
{
	struct cook_stage_blob_t
	{
		vector_t<u8> bytes;
		u8			 stage = 0;
	};

	struct cook_compile_variant_t
	{
		vector_t<cook_stage_blob_t> stages;
	};

	struct cook_pso_variant_t
	{
		shader_desc_t desc					= {};
		u32			  variant_flags			= 0;
		u8			  compile_variant_index = 0;
	};

	class shader_cook_variants_t
	{
	public:
		static bool cook_object_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_sprite_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_particle_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_post_process_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_ui_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_ui_text_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_skybox_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_deferred_lighting_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_post_combiner_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_texture_blit_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_gizmo_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_line_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_text_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_triangle_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_texture_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_compute_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_ui_default(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_ui_lcd_text(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_ui_text_grayscale(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_ui_sdf(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
	};
}
