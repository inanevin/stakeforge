// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_inspector.hpp"

namespace sfg
{
	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
	}
}
