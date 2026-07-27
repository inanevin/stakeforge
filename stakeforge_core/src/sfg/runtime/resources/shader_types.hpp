// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	enum class shader_type_e : u8
	{
		invalid,
		editor_ui_default,
		editor_ui_lcd_text,
		editor_ui_sdf,
		editor_ui_text_grayscale,
		object_shader,
		post_process_shader,
		ui_shader,
		ui_text_shader,
		deferred_lighting,
		post_combiner,
		texture_blit,
		editor_gizmo,
		debug_line,
		debug_text,
		ssao,
		ssao_upsample,
		bloom_downsample,
		bloom_upsample,
		debug_triangle,
		skybox_shader,
		clustered_light_culling,
		reflection_specular_prefilter,
		reflection_diffuse_sh,
		sprite_lit_shader,
		sprite_unlit_shader,
		particle_shader,
		count,
	};

	enum shader_variant_flags_e
	{
		shader_variant_flags_skinned					  = 1 << 0,
		shader_variant_flags_alpha_cutoff				  = 1 << 1,
		shader_variant_flags_z_prepass					  = 1 << 2,
		shader_variant_flags_double_sided				  = 1 << 3,
		shader_variant_flags_shadow_rendering			  = 1 << 4,
		shader_variant_flags_selection_outline			  = 1 << 5,
		shader_variant_flags_id_write					  = 1 << 6,
		shader_variant_flags_gbuffer					  = 1 << 7,
		shader_variant_flags_particle_alpha				  = 1 << 8,
		shader_variant_flags_particle_premultiplied_alpha = 1 << 9,
		shader_variant_flags_particle_additive			  = 1 << 10,
	};

	SFG_DEFINE_TYPE_ID(shader_type_e);

	struct shader_type_reflection_t
	{
		shader_type_reflection_t();
	};

	inline shader_type_reflection_t g_reflect_shader_type;
}
