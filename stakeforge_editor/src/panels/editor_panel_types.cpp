// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_types.hpp"
#include <sfg/common/hashing.hpp>

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

	editor_panel_type_e editor_panel_type_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);
		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
		{
			const editor_panel_type_e type = static_cast<editor_panel_type_e>(i);
			if (TO_SID(editor_panel_type_to_string(type)) == id)
				return type;
		}
		return editor_panel_type_e::max;
	}
}
