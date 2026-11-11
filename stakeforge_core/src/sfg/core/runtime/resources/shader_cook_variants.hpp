// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/data/string.hpp"
#include "core/data/vector.hpp"
#include "core/gfx/common/shader_description.hpp"

namespace sfg
{
	enum class shader_type_e : u8;

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

	struct shader_custom_cook_request_t
	{
		const string_t&			  type_name;
		const string_t&			  source;
		const vector_t<string_t>& include_paths;
		const char*				  source_path = nullptr;
	};

	struct SFG_API shader_custom_cook_result_t
	{
		vector_t<cook_compile_variant_t> compiles = {};
		vector_t<cook_pso_variant_t>	 psos	  = {};

		bool is_valid() const;
	};

	class SFG_API shader_cook_variants_t
	{
	public:
		static bool cook_custom_fallback(vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_object_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_sprite_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_particle_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_ui_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos, shader_type_e type);
		static bool cook_skybox_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_deferred_lighting_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_post_combiner_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_texture_blit_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_gizmo_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_line_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_text_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_triangle_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_editor_icon_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_debug_texture_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
		static bool cook_compute_shader(const string_t& source, const vector_t<string_t>& include_paths, vector_t<cook_compile_variant_t>& out_compiles, vector_t<cook_pso_variant_t>& out_psos);
	};
}
