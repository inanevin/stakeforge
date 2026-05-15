// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_profiling.hpp"

namespace sfg
{
	editor_panel_profiling_t::editor_panel_profiling_t()
	{
		set_type(editor_panel_type_e::profiling);
		set_title(editor_panel_type_to_string(editor_panel_type_e::profiling));
	}
}
