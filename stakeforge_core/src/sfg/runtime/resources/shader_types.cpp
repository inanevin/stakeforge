// Copyright (c) 2025 Inan Evin

#include "shader_types.hpp"

namespace sfg
{
	void from_json(const nlohmann::json& j, shader_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "editor_ui_default")
			t = shader_type_e::editor_ui_default;
		else if (s == "editor_ui_lcd_text")
			t = shader_type_e::editor_ui_lcd_text;
		else if (s == "editor_ui_sdf")
			t = shader_type_e::editor_ui_sdf;
		else
			t = shader_type_e::invalid;
	}

}
