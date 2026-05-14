// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_types.hpp"

namespace sfg
{
	const char* editor_panel_type_to_string(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return "Entities";
		case editor_panel_type_e::assets:
			return "Assets";
		case editor_panel_type_e::log:
			return "Log";
		case editor_panel_type_e::world:
			return "World";
		case editor_panel_type_e::inspector:
			return "Inspector";
		case editor_panel_type_e::animation:
			return "Animation";
		case editor_panel_type_e::profiling:
			return "Profiling";
		default:
			return "";
		}
	}
}
