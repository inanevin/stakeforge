// Copyright (c) 2025 Inan Evin

#include "shader_types.hpp"

namespace sfg
{
	void from_json(const nlohmann::json& j, shader_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "opaque_shader")
			t = shader_type_e::opaque_shader;
		else if (s == "transparent_shader")
			t = shader_type_e::transparent_shader;
		else if (s == "post_process_shader")
			t = shader_type_e::post_process_shader;
		else if (s == "ui_shader")
			t = shader_type_e::ui_shader;
		else if (s == "ui_text_shader")
			t = shader_type_e::ui_text_shader;
		else if (s == "editor_ui_default")
			t = shader_type_e::editor_ui_default;
		else if (s == "editor_ui_lcd_text")
			t = shader_type_e::editor_ui_lcd_text;
		else if (s == "editor_ui_text_grayscale")
			t = shader_type_e::editor_ui_text_grayscale;
		else if (s == "editor_ui_sdf")
			t = shader_type_e::editor_ui_sdf;
		else
			t = shader_type_e::invalid;
	}

}
