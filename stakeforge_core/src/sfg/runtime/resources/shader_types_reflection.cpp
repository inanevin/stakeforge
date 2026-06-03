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

#include "shader_types_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <iterator>

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
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = shader_type_values, .size = std::size(shader_type_values)},
			.name		 = "shader_type_e",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(shader_type_e),
			.alignment	 = alignof(shader_type_e),
		});
	}
}
