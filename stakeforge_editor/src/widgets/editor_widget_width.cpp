// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widget_width.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>

namespace sfg
{
	void apply_editor_widget_width(ui::layout_in_t& in, const editor_widget_width_config_t& width)
	{
		if (width.mode == editor_widget_width_e::fixed)
		{
			in.size_mode_x	= ui::axis_mode_e::fixed;
			in.size_value.x = width.value;
			return;
		}

		in.size_mode_x	= ui::axis_mode_e::parent_relative;
		in.size_value.x = 1.0f;
	}
}
