// Copyright (c) 2025 Inan Evin

#include "shader_types.hpp"

namespace sfg
{
	void from_json(const nlohmann::json& j, shader_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "editor_ui_default")
			t = shader_type_e::editor_ui_default;
		else
			t = shader_type_e::invalid;
	}

}
