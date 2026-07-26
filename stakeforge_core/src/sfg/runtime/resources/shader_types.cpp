// Copyright (c) 2025 Inan Evin

#include "shader_types.hpp"
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	shader_type_reflection_t::shader_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "shader_type_e",
			.fields =
				{
					{.name = "invalid", .display_name = "Invalid"},
					{.name = "editor_ui_default", .display_name = "Editor UI Default"},
					{.name = "editor_ui_lcd_text", .display_name = "Editor UI LCD Text"},
					{.name = "editor_ui_sdf", .display_name = "Editor UI SDF"},
					{.name = "editor_ui_text_grayscale", .display_name = "Editor UI Text Grayscale"},
					{.name = "object_shader", .display_name = "Object Shader"},
					{.name = "post_process_shader", .display_name = "Post Process Shader"},
					{.name = "ui_shader", .display_name = "UI Shader"},
					{.name = "ui_text_shader", .display_name = "UI Text Shader"},
					{.name = "deferred_lighting", .display_name = "Deferred Lighting"},
					{.name = "post_combiner", .display_name = "Post Combiner"},
					{.name = "texture_blit", .display_name = "Texture Blit"},
					{.name = "editor_gizmo", .display_name = "Editor Gizmo"},
					{.name = "debug_line", .display_name = "Debug Line"},
					{.name = "debug_text", .display_name = "Debug Text"},
					{.name = "ssao", .display_name = "SSAO"},
					{.name = "ssao_upsample", .display_name = "SSAO Upsample"},
					{.name = "bloom_downsample", .display_name = "Bloom Downsample"},
					{.name = "bloom_upsample", .display_name = "Bloom Upsample"},
					{.name = "debug_triangle", .display_name = "Debug Triangle"},
					{.name = "skybox_shader", .display_name = "Skybox Shader"},
					{.name = "clustered_light_culling", .display_name = "Clustered Light Culling"},
					{.name = "reflection_specular_prefilter", .display_name = "Reflection Specular Prefilter"},
					{.name = "reflection_diffuse_sh", .display_name = "Reflection Diffuse SH"},
				},
			.type_id   = type_id_t<shader_type_e>::value,
			.size	   = sizeof(shader_type_e),
			.alignment = alignof(shader_type_e),
			.flags	   = reflected_type_flag_enum,
		});
	}
}
