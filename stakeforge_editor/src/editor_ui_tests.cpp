// Copyright (c) 2025 Inan Evin

#include "editor_ui_tests.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_ui_tests_t::make_test_general(ui::ui_context& ui)
	{
		ui::layout_tree_t& tree	 = ui.get_tree();
		ui::paint_layer_t& paint = ui.get_paint();

		const ui::widget_id_t col = ui.make_column(ui.get_root());
		ui.make_button(col, "test_buttonssssssssss", "editor/fonts/Roboto-Regular.ttf"_hs);
		ui.make_label(col, "what the fuck", "editor/fonts/Roboto-Regular.ttf"_hs);

		{
		}

		return;

		for (u32 i = 0; i < 4; i++)
		{
			const ui::widget_id_t panel = ui.make_panel(col);
			ui::layout_in_t&	  in	= tree.in(panel);

			in.pos_mode_x = ui::pos_mode_e::flow;
			in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			in.pos_value  = {0.0f, 0.0f};

			in.size_mode_x = ui::axis_mode_e::fill;
			in.size_mode_y = ui::axis_mode_e::parent_relative;
			in.size_value  = {.6f, 0.5f};

			ui::vg_rect_paint_t& rect = paint.def(panel).rect;
			rect.fill_color_a		  = {0.2f, 0.1f, 0.1f, 1.0f};

			paint.set_rect(panel, rect);
		}
	}

	void editor_ui_tests_t::make_test_text(ui::ui_context& ui)
	{
		const resource_handle_t font = "editor/fonts/Roboto-Regular.ttf"_hs;

		ui::layout_tree_t&	  tree = ui.get_tree();
		const ui::widget_id_t col  = ui.make_column(ui.get_root());

		ui::layout_in_t& col_in = tree.in(col);
		col_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		col_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		col_in.size_value		= {1.0f, 1.0f};
		col_in.child_margins	= {16.0f, 16.0f, 16.0f, 16.0f};

		const f32 sizes[] = {10.0f, 12.0f, 13.0f, 14.0f, 16.0f, 20.0f, 24.0f, 32.0f, 48.0f, 64.0f, 128.0f};
		for (f32 sz : sizes)
			ui.make_label(col, "The quick brown fox jumps over the lazy dog", font, sz);
	}
}
