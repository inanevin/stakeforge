// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_animation.hpp"

namespace sfg
{
	editor_panel_animation_t::editor_panel_animation_t()
	{
		set_type(editor_panel_type_e::animation);
		set_title(editor_panel_type_to_string(editor_panel_type_e::animation));
	}
}
