// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/type_id.hpp"

#include "core/common/size_definitions.hpp"

namespace sfg
{
	enum class shader_type_e : u8
	{
		invalid,
		editor_ui,
		editor_ui_text,
		reserved_3,
		reserved_4,
		object_shader,
		reserved_6,
		reserved_7,
		canvas_ui_text,
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
		debug_texture,
		canvas_ui,
		editor_icon,
		canvas_ui_texture,
		editor_ui_texture,
		custom_shader,
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
		shader_variant_flags_canvas_hdr					  = 1 << 11,
		shader_variant_flags_canvas_ldr					  = 1 << 12,
		shader_variant_flags_text_lcd					  = 1 << 13,
		shader_variant_flags_canvas_3d					  = 1 << 15,
		shader_variant_flags_text_sdf					  = 1 << 14,
	};

	SFG_DEFINE_TYPE_ID(shader_type_e);

	struct shader_type_reflection_t
	{
		shader_type_reflection_t();
	};

}
