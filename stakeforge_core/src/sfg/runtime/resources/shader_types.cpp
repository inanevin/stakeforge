// Copyright (c) 2025 Inan Evin

#include "shader_types.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t shader_type_values[] = {
			{.name = "invalid", .display_name = "Invalid", .value = static_cast<i64>(shader_type_e::invalid)},
			{.name = "editor_ui_default", .display_name = "Editor UI Default", .value = static_cast<i64>(shader_type_e::editor_ui_default)},
			{.name = "editor_ui_lcd_text", .display_name = "Editor UI LCD Text", .value = static_cast<i64>(shader_type_e::editor_ui_lcd_text)},
			{.name = "editor_ui_sdf", .display_name = "Editor UI SDF", .value = static_cast<i64>(shader_type_e::editor_ui_sdf)},
			{.name = "editor_ui_text_grayscale", .display_name = "Editor UI Text Grayscale", .value = static_cast<i64>(shader_type_e::editor_ui_text_grayscale)},
			{.name = "opaque_shader", .display_name = "Opaque Shader", .value = static_cast<i64>(shader_type_e::opaque_shader)},
			{.name = "transparent_shader", .display_name = "Transparent Shader", .value = static_cast<i64>(shader_type_e::transparent_shader)},
			{.name = "post_process_shader", .display_name = "Post Process Shader", .value = static_cast<i64>(shader_type_e::post_process_shader)},
			{.name = "ui_shader", .display_name = "UI Shader", .value = static_cast<i64>(shader_type_e::ui_shader)},
			{.name = "ui_text_shader", .display_name = "UI Text Shader", .value = static_cast<i64>(shader_type_e::ui_text_shader)},
		};
	}

	shader_type_reflection_t::shader_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<shader_type_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = shader_type_values, .size = std::size(shader_type_values)},
			.name		 = "shader_type_e",
			.type_id	 = type_id_t<shader_type_e>::value,
			.size		 = sizeof(shader_type_e),
			.alignment	 = alignof(shader_type_e),
		});
	}
}
